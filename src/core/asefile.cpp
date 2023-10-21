#include "asefile.h"

#include "utils.h"

#include "containers/common.h"

#include "zlib.h"

// NOTE: Aseprite file spec is hosted at https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md
// There is also copy of it at docs/ase-file-specs.md
// If the spec changes, please update also the copy of the doc file

using namespace bstr::core;

namespace bstr::core {


using BYTE = u8;
using WORD = u16;
using SHORT = s16;
using DWORD = u32;
using LONG = s32;
struct FIXED {
	u16 integer_part;
	u16 fractional_part;
};
using FLOAT = f32;
using DOUBLE = f64;
using QWORD = u64;
using LONG64 = s64;
//struct STRING {
//	WORD length;
//	BYTE characters[length];
//};

struct POINT {
	LONG x;
	LONG y;
};
struct SIZE {
	LONG width;
	LONG height;
};
struct RECT {
	POINT origin;
	SIZE size;
};
struct UUID {
	BYTE uuid[16];
};

#pragma pack(1)
struct AseHeader {
	DWORD       file_size;
	WORD        magic_number; // 0xA5E0
	WORD        frames;
	WORD        width_in_pixels;
	WORD        height_in_pixels;
	WORD        color_depth; // bits per pixel
							 // 32 bpp = RGBA
							 // 16 bpp = Grayscale
							 // 8 bpp = Indexed
	DWORD       flags; // 1 = Layer opacity has valid value

	WORD        speed; // milliseconds between frame, like in FLC files
					   // DEPRECATED : You should use the frame duration field
	                   // from each frame header
	DWORD       zero_value0;
	DWORD       zero_value1;
	BYTE        palette_transparent_index; // Palette entry (index) which represent transparent color
							               // in all non - background layers(only for Indexed sprites).

	BYTE ignore[3];

	WORD        num_colors; //  Number of colors(0 means 256 for old sprites)
	BYTE        pixel_width; // Pixel width(pixel ratio is "pixel width/pixel height").
							 // If this or pixel height field is zero, pixel ratio is 1:1
	BYTE        pixel_heigt;
	SHORT       x_position_of_grid;
	SHORT       y_position_of_grid;
	WORD        grid_with; // Grid width(zero if there is no grid, grid size
						   // is 16x16 on Aseprite by default)
	WORD        grid_height; // Grid height(zero if there is no grid)

	BYTE		ignored2[84];
};
static_assert(sizeof(AseHeader) == 128, "AseHeader should be 128 bytes as per the spec");

#pragma pack(1)
struct AseFrameHeader {
	DWORD       bytes; // Bytes in this frame
	WORD        magic_number; // Magic number(always 0xF1FA)
	WORD        old_number_of_chunks; // Old field which specifies the number of "chunks"
									  // in this frame. If this value is 0xFFFF, we might
									  // have more chunks to read in this frame
									  // (so we have to use the new field)
	WORD        frame_duration_ms; // Frame duration(in milliseconds)
	BYTE		future_value[2]; // For future(set to zero)
	DWORD       new_number_of_chunks; // New field which specifies the number of "chunks"
									  // in this frame(if this is 0, use the old field)
};
static_assert(sizeof(AseFrameHeader) == 16, "AseFrameHeader should be 16 as per the spec");

#pragma pack(1)
struct AseFrameChunk {
	DWORD       chunk_size;	   // Chunk size
	WORD        chunk_type;	   // Chunk type
	//BYTE		chunk_data[1]; // Chunk data
};
static_assert(sizeof(AseFrameChunk) == 6, "AseFrameChunk header should be 6 as per the spec");

#define CHUNK_TYPES(X) \
	X(OldPalette, 0x0004) \
	X(OldPalette2, 0x0011) \
	X(Layer, 0x2004) \
	X(Cel, 0x2005) \
	X(CelExtra, 0x2006) \
	X(Color, 0x2007) \
	X(ExternalFiles, 0x2008) \
	X(Mask, 0x2016) \
	X(Path, 0x2017) \
	X(Tags, 0x2018) \
	X(Palette, 0x2019) \
	X(UserData, 0x2020) \
	X(Slice, 0x2022) \
	X(TileSet, 0x2023)

enum ChunkType : WORD {
	#define X(type, id) ChunkType_##type = id,
	CHUNK_TYPES(X)
	#undef X
};

static string chunk_type_name(WORD type) {
	#define X(type, id) { id, #type },
	static hash_map<WORD, string> map {
		CHUNK_TYPES(X)
	};
	#undef X
	auto it = map.find(type);
	if (it != map.end()) {
		return it->second;
	} else {
		return "UNKNOWN";
	}
}

#pragma pack(1)
struct AseLayerChunk {
	WORD        flags;  // 1 = Visible
						// 2 = Editable
						// 4 = Lock movement
						// 8 = Background
						// 16 = Prefer linked cels
						// 32 = The layer group should be displayed collapsed
						// 64 = The layer is a reference layer
	WORD        layer_type; // 0 = Normal(image) layer
							// 1 = Group
							// 2 = Tilemap
	WORD        layer_child_level; // Layer child level(see NOTE.1)
	// The child level is used to show the relationship of this layer with the last one read, for example:
	// 
	// Layer name and hierarchy      Child Level
	// 	-----------------------------------------------
	// 	- Background                 0
	// 	| - Layer1                   1
	// 	- Foreground                 0
	// 	| - My set1                  1
	// 	| | - Layer2                 2
	// 	| - Layer3                   1

	WORD        ignored_default_layer_width; // Default layer width in pixels(ignored)
	WORD        ignored_default_layer_height; // Default layer height in pixels(ignored)
	WORD        blend_mode; // Blend mode(always 0 for layer set)
							// Normal = 0
							// Multiply = 1
							// Screen = 2
							// Overlay = 3
							// Darken = 4
							// Lighten = 5
							// Color Dodge = 6
							// Color Burn = 7
							// Hard Light = 8
							// Soft Light = 9
							// Difference = 10
							// Exclusion = 11
							// Hue = 12
							// Saturation = 13
							// Color = 14
							// Luminosity = 15
							// Addition = 16
							// Subtract = 17
							// Divide = 18
	BYTE        opacity; // Note : valid only if file header flags field has bit 1 set
	BYTE		for_future[3];//     For future(set to zero)
	//STRING      layer_name;
	//DWORD		tileset_index;
};

enum CelChunkType : WORD {
	CelChunk_RawImageData = 0,
	CelChunk_LinkedCel = 1,
	CelChunk_CompressedImage = 2,
	CelChunk_CompressedTilemap = 3,
};

#pragma pack(1)
struct AseCelChunk {
	WORD        layer_index; // Layer index(see NOTE.2)
	SHORT       x_position; // X position
	SHORT       y_position; // Y position
	BYTE        opacity_level; // Opacity level
	WORD        cel_type; // Cel Type
						  // 0 - Raw Image Data(unused, compressed image is preferred)
						  // 1 - Linked Cel
						  // 2 - Compressed Image
						  // 3 - Compressed Tilemap
	SHORT       z_index; // Z - Index(see NOTE.5)
						 // 0 = default layer ordering
						 // + N = show this cel N layers later
						 // - N = show this cel N layers back
	BYTE		for_future[5]; // For future(set to zero)
	// + For cel type = 0 (Raw Image Data)
	// WORD      Width in pixels
	// WORD      Height in pixels
	// PIXEL[]   Raw pixel data : row by row from top to bottom,
	// for each scanline read pixels from left to right.
	// 	+ For cel type = 1 (Linked Cel)
	// 	WORD      Frame position to link with
	// 	+ For cel type = 2 (Compressed Image)
	// 	WORD      Width in pixels
	// 	WORD      Height in pixels
	// 	PIXEL[]   "Raw Cel" data compressed with ZLIB method(see NOTE.3)
	// 	+ For cel type = 3 (Compressed Tilemap)
	// 	WORD      Width in number of tiles
	// 	WORD      Height in number of tiles
	// 	WORD      Bits per tile(at the moment it's always 32-bit per tile)
	// 		DWORD     Bitmask for tile ID(e.g. 0x1fffffff for 32 - bit tiles)
	// 		DWORD     Bitmask for X flip
	// 		DWORD     Bitmask for Y flip
	// 		DWORD     Bitmask for 90CW rotation
	// 		BYTE[10]  Reserved
	// 		TILE[]    Row by row, from top to bottom tile by tile
	// 		compressed with ZLIB method(see NOTE.3)
};

class AsepriteLoader {
public:
	explicit AsepriteLoader(const char* filename, non_null<AseFile> ase_file)
		: m_filename(filename)
		, m_ase_file(ase_file)
	{
	}
	
	void load();

private:
	template<typename T>
	void load_from_file(T& dest)
	{
		ASSERT(m_file_pointer + sizeof(T) <= m_file_data.size(), "");
		memcpy(&dest, m_file_data.data() + m_file_pointer, sizeof(T));
		m_file_pointer += sizeof(T);
	}

	string load_string_from_file()
	{
		WORD length;
		load_from_file(length);
		
		ASSERT(m_file_pointer + length <= m_file_data.size(), "");

		string str;
		str.resize(length);

		memcpy(str.data(), m_file_data.data() + m_file_pointer, length);
		m_file_pointer += length;

		return str;

	}
	
	template<typename T>
	vector<T> load_array_from_file(usz length)
	{
		ASSERT(m_file_pointer + length * sizeof(T) <= m_file_data.size(), "");
		vector<T> arr;
		arr.resize(length);
		memcpy(arr.data(), m_file_data.data() + m_file_pointer, length * sizeof(T));
		m_file_pointer += length * sizeof(T);
		return arr;
	}

	usz bytes_per_pixel() const { return m_header.color_depth / 8; }

	void load_header();
	void load_frames();
	void load_layer(DWORD chunk_size);
	void load_cel(DWORD chunk_size);

	non_null<AseFile> m_ase_file;

	const char* m_filename;

	AseHeader m_header{};

	usz m_file_pointer = 0;
	vector<u8> m_file_data;
};

AseFile AseFile::load_from_file(const char* filename)
{
	AseFile ase_file;
	auto loader = AsepriteLoader{ filename, &ase_file };
	loader.load();
	return ase_file;
}

void AsepriteLoader::load()
{
	m_file_data = read_entire_file_as_bytes(m_filename);
	load_header();
	load_frames();
}

void AsepriteLoader::load_header()
{
	load_from_file(m_header);
	ASSERT(m_header.file_size == m_file_data.size(), "");
	ASSERT(m_header.magic_number == 0xA5E0, "");
	ASSERT(m_header.zero_value0 == 0, "");
	ASSERT(m_header.zero_value1 == 0, "");
}

void AsepriteLoader::load_frames()
{
	while(m_file_pointer + sizeof(AseFrameHeader) <= m_file_data.size()) {
		AseFrameHeader frame_header;
		load_from_file(frame_header);
		ASSERT(frame_header.magic_number == 0xF1FA, "");
		ASSERT(m_file_pointer + frame_header.bytes - sizeof(AseFrameHeader) <= m_file_data.size(), "");
		DWORD number_of_chunks = frame_header.new_number_of_chunks == 0
			? frame_header.old_number_of_chunks
			: frame_header.new_number_of_chunks;
		for (usz chunk_index = 0; chunk_index < number_of_chunks; ++chunk_index) {
			AseFrameChunk chunk_header;
			load_from_file(chunk_header);
			ASSERT(chunk_header.chunk_size >= sizeof(AseFrameChunk), "");
			chunk_header.chunk_size -= sizeof(AseFrameChunk);

			switch (chunk_header.chunk_type) {
			case ChunkType_Layer: load_layer(chunk_header.chunk_size); break;
			case ChunkType_Cel: load_cel(chunk_header.chunk_size); break;

			case ChunkType_OldPalette:
			case ChunkType_OldPalette2:
			case ChunkType_CelExtra:
			case ChunkType_Color:
			case ChunkType_ExternalFiles:
			case ChunkType_Mask:
			case ChunkType_Path:
			case ChunkType_Tags:
			case ChunkType_Palette:
			case ChunkType_UserData:
			case ChunkType_Slice:
			case ChunkType_TileSet:
				LOG_WARN("Encountered chunk_type: {}, which is not handled", chunk_type_name(chunk_header.chunk_type));
				m_file_pointer += chunk_header.chunk_size;
				break;
			
			default:
				ASSERT(false, "Unhandled ase chunk type");
			}

		}
	}
}

void AsepriteLoader::load_layer(DWORD chunk_size)
{
	auto start_pointer = m_file_pointer;
	
	AseLayerChunk layer_chunk;
	load_from_file(layer_chunk);

	string layer_name = load_string_from_file();

	if (layer_chunk.layer_type == 2) {
		DWORD tileset_index;
		load_from_file(tileset_index);
		ASSERT(false, "");
	}

	auto end_pointer = m_file_pointer;
	ASSERT((end_pointer - start_pointer) == chunk_size, "");
}

void AsepriteLoader::load_cel(DWORD chunk_size)
{
	auto start_pointer = m_file_pointer;

	AseCelChunk cel_chunk;
	load_from_file(cel_chunk);

	switch (cel_chunk.cel_type) {
		case CelChunk_RawImageData: {
			// + For cel type = 0 (Raw Image Data)
			// WORD      Width in pixels
			// WORD      Height in pixels
			// PIXEL[]   Raw pixel data : row by row from top to bottom,
			//		     for each scanline read pixels from left to right.
			WORD width_in_pixels;
			load_from_file(width_in_pixels);
			WORD height_in_pixels;
			load_from_file(height_in_pixels);
			vector<BYTE> pixels_bytes = load_array_from_file<BYTE>(width_in_pixels * height_in_pixels * bytes_per_pixel());
			ASSERT(false, "");
		} break;

		case CelChunk_LinkedCel: {
			// 	+ For cel type = 1 (Linked Cel)
			// 	WORD      Frame position to link with
			WORD frame_link_position;
			load_from_file(frame_link_position);
			ASSERT(false, "");
		} break;
		
		case CelChunk_CompressedImage: {
			// 	+ For cel type = 2 (Compressed Image)
			// 	WORD      Width in pixels
			// 	WORD      Height in pixels
			// 	PIXEL[]   "Raw Cel" data compressed with ZLIB method(see NOTE.3)
			WORD width_in_pixels;
			load_from_file(width_in_pixels);
			WORD height_in_pixels;
			load_from_file(height_in_pixels);
			vector<BYTE> pixel_bytes;
			// ZEXTERN int ZEXPORT uncompress(Bytef * dest, uLongf * destLen,
			// 	const Bytef * source, uLong sourceLen);
			// /*
			// 	 Decompresses the source buffer into the destination buffer.  sourceLen is
			//    the byte length of the source buffer.  Upon entry, destLen is the total size
			//    of the destination buffer, which must be large enough to hold the entire
			//    uncompressed data.  (The size of the uncompressed data must have been saved
			//    previously by the compressor and transmitted to the decompressor by some
			//    mechanism outside the scope of this compression library.) Upon exit, destLen
			//    is the actual size of the uncompressed data.
			// 
			// 	 uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
			//    enough memory, Z_BUF_ERROR if there was not enough room in the output
			//    buffer, or Z_DATA_ERROR if the input data was corrupted or incomplete.  In
			//    the case where there is not enough room, uncompress() will fill the output
			//    buffer with the uncompressed data up to that point.
			// */
			pixel_bytes.resize(width_in_pixels * height_in_pixels * bytes_per_pixel());
			Bytef* dst = (Bytef*)pixel_bytes.data();
			uLongf dst_len = (uLongf)pixel_bytes.size();
			Bytef* src = m_file_data.data() + m_file_pointer;
			auto read_so_far = m_file_pointer - start_pointer;
			uLong src_len = (uLong)(chunk_size - read_so_far);
			int res = uncompress(dst, &dst_len, src, src_len);
			switch (res) {
			case Z_OK:
				break;
			case Z_MEM_ERROR:
				LOG_ERROR("there was not enough memory when uncompressing ase file {}", m_filename);
				ASSERT(false, "");
				break;
			case Z_BUF_ERROR:
				LOG_ERROR("there was not enough room in the output buffer when uncompressing ase file {}", m_filename);
				ASSERT(false, "");
				break;
			case Z_DATA_ERROR:
				LOG_ERROR("the input data was corrupted or incomplete when uncompressing ase file {}", m_filename);
				break;
			}
		} break;
		
		case CelChunk_CompressedTilemap: {
			// 	+ For cel type = 3 (Compressed Tilemap)
			#pragma pack(1)
			struct {
				WORD      width_in_number_of_tiles; // Width in number of tiles
				WORD      height_in_number_of_tiles; // Height in number of tiles
				WORD      bits_per_tile; // Bits per tile(at the moment it's always 32-bit per tile)
				DWORD     bitmask_for_tile_id; // Bitmask for tile ID(e.g. 0x1fffffff for 32 - bit tiles)
				DWORD     bitmask_for_x_flip; // Bitmask for X flip
				DWORD     bitmask_for_y_flip; // Bitmask for Y flip
				DWORD     bitmask_for_90CW_rotation; // Bitmask for 90CW rotation
				BYTE      reserved[10];
			} compressed_tilemap;
			load_from_file(compressed_tilemap);
			//  TILE[]    Row by row, from top to bottom tile by tile
			//			  compressed with ZLIB method(see NOTE.3)
			// load_array_from_file<TILE>(??)
			ASSERT(false, "");
		} break;
	}

	auto end_pointer = m_file_pointer;
	ASSERT((end_pointer - start_pointer) == chunk_size, "");
}

}