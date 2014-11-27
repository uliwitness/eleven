//
//  eleven_database.h
//  interconnectserver
//
//  Created by Uli Kusterer on 2014-11-25.
//  Copyright (c) 2014 Uli Kusterer. All rights reserved.
//

#include "eleven_database.h"
#include "libscrypt.h"


using namespace eleven;


std::string	database::hash( std::string inPassword )
{
	char			outbuf[SCRYPT_MCF_LEN] = {0};
	libscrypt_hash(outbuf, inPassword.c_str(), SCRYPT_N, SCRYPT_r, 4);
	
	return std::string( outbuf, SCRYPT_MCF_LEN );
}


bool	database::hash_password_equal( std::string inHash, std::string inPassword )
{
	char			actualPasswordHash[SCRYPT_MCF_LEN] = {0};
	inHash.copy(actualPasswordHash, SCRYPT_MCF_LEN);
	if( libscrypt_check( actualPasswordHash, inPassword.c_str() ) <= 0 )
		return false;
	else
		return true;
}
