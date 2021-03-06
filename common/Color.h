#ifndef COMMON_COLOR_H
#define COMMON_COLOR_H

#include <cstdint>
#include <cstring>

/**
*	Makes a 4 byte RGB color. A is set to 255.
*/
#define MAKE_RGB( r, g, b ) ( ( ( r & 0xFF ) ) | ( ( g & 0xFF ) << 8 ) | ( ( b & 0xFF ) << 16 ) | ( 0xFF << 24 ) )

/**
*	Represents a HUD color. - Solokiller
*	Note: not the same as vgui::Color. That has a scheme color member in addition to the RGBA data.
*/
struct Color final
{
	static const size_t NUM_COMPONENTS = 4;

	/**
	*	Constructor.
	*	Creates a color from a 32 bit color value. Layout is | Unused | R | G | B |
	*	Defaults to white.
	*/
	Color( uint32_t color32 = MAKE_RGB( 255, 255, 255 ) )
		: color32( color32 )
	{
	}

	/**
	*	Constructor.
	*	Creates a color from a red, green and blue color.
	*/
	Color( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 )
		: r( r )
		, g( g )
		, b( b )
		, a( a )
	{
	}

	Color( const Color& other ) = default;
	Color& operator=( const Color& other ) = default;

	/**
	*	Unpacks this color into r, g and b components.
	*	Obsolete. Directly accessing the color components is easier.
	*/
	void UnpackRGB( int& r, int& g, int& b ) const
	{
		r = this->r;
		g = this->g;
		b = this->b;
	}

	/**
	*	Sets a color from a 32 bit color value. Layout is | Unused | R | G | B |
	*/
	void Set( uint32_t color32 )
	{
		this->color32 = color32;
	}

	/**
	*	Sets a color from a red, green and blue color.
	*/
	void Set( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 )
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	uint8_t operator[]( const size_t uiIndex ) const
	{
		return reinterpret_cast<const uint8_t*>( this )[ uiIndex ];
	}

	uint8_t& operator[]( const size_t uiIndex )
	{
		return reinterpret_cast<uint8_t*>( this )[ uiIndex ];
	}

	union
	{
		uint32_t color32;

		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
	};
};

/**
*	Default main HUD color.
*/
#define RGB_YELLOWISH MAKE_RGB( 255, 160, 0 )

/**
*	Default empty / nearly empty item color.
*/
#define RGB_REDISH MAKE_RGB( 255, 16, 16 )

/**
*	Default ammo bar color in weapon list.
*/
#define RGB_GREENISH MAKE_RGB( 0, 160, 0 )

#endif //COMMON_COLOR_H
