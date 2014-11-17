//
//  eleven_ini_file.cpp
//  eleven
//
//  Created by Uli Kusterer on 2014-11-17.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_ini_file.h"
#include <cstdio>


using namespace eleven;


ini_file::ini_file( std::string filePath )
	: mValid(false)
{
	FILE*			iniFile = fopen( filePath.c_str(), "r" );
	std::string		keyStr;
	std::string		valueStr;
	bool			parsingKey = true;
	
	if( !iniFile )
	{
		return;
	}
	
	while( true )
	{
		char	theCh = 0;
		if( fread( &theCh, sizeof(theCh), 1, iniFile ) != 1 )
		{
			break;
		}
		if( !parsingKey && (theCh == '\n' || theCh == '\r') )
		{
			parsingKey = true;
			if( keyStr.size() > 0 )
				mSettings[keyStr] = valueStr;
			keyStr.clear();
			valueStr.clear();
		}
		else if( !parsingKey )
		{
			valueStr.append( 1, theCh );
		}
		else if( parsingKey && theCh == '=' )
		{
			parsingKey = false;
		}
		else if( parsingKey )
		{
			keyStr.append( 1, theCh );
		}
	}
	
	if( !parsingKey && keyStr.size() > 0 )	// Leftover key/value pair? Write it, too.
	{
		mSettings[keyStr] = valueStr;
	}
	
	mValid = true;
	fclose(iniFile);
}