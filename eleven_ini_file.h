//
//  eleven_ini_file.h
//  eleven
//
//  Created by Uli Kusterer on 2014-11-17.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#ifndef __eleven__eleven_ini_file__
#define __eleven__eleven_ini_file__

/* Tiny, very simple, immutable INI file parser. All it does is parse single-line
	key-value pairs, separated by an equals sign, case-sensitive.
	Whitespace is included as part of the key or value. Only the equals
	sign itself and the line break at the end are currently stripped.
	
	We may fix this in the future or switch to an open source library
	if I find one I like. */


#include <string>
#include <map>
#include <cstdlib>


namespace eleven
{

	class ini_file
	{
	public:
		explicit ini_file();
		
		bool	open( std::string filePath );
		bool	valid()	{ return mValid; };
		
		std::string		setting( const std::string inKey )	{ return mSettings[inKey]; };
		long			setting_as_long( const std::string inKey )	{ return std::strtol( mSettings[inKey].c_str(), nullptr, 10 ); };
		unsigned int	setting_as_uint( const std::string inKey )	{ return (unsigned int) std::strtoul( mSettings[inKey].c_str(), nullptr, 10 ); };
		
	protected:
		std::map<std::string,std::string>	mSettings;
		bool								mValid;
	};

}

#endif /* defined(__eleven__eleven_ini_file__) */
