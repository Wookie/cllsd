/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

#include "debug.h"
#include "macros.h"
#include "hashtable.h"
#include "array.h"
#include "llsd.h"

/* constants */
llsd_t const undefined =
{
	.type_ = LLSD_UNDEF,
	.value.bool_ = FALSE
};

llsd_uuid_t const zero_uuid = 
{ 
	.bits = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

llsd_string_t const false_string = 
{
	.dyn = FALSE,
	.escaped = FALSE,
	.str = "false"
};
llsd_string_t const true_string = 
{
	.dyn = FALSE,
	.escaped = FALSE,
	.str = "true"
};

uint8_t zero_data [] = { '0' };
llsd_binary_t const false_binary =
{
	.dyn = FALSE,
	.size = 1,
	.data = zero_data
};
uint8_t one_data[] = { '1' };
llsd_binary_t const true_binary =
{
	.dyn = FALSE,
	.size = 1,
	.data = one_data
};
llsd_binary_t const empty_binary =
{
	.dyn = FALSE,
	.size = 0,
	.data = 0
};
llsd_uri_t const empty_uri = 
{
	.dyn = FALSE,
	.uri = ""
};


#define FNV_PRIME (0x01000193)
static uint32_t fnv_key_hash(void const * const key)
{
	int i;
	llsd_t * llsd = (llsd_t*)key;
    uint32_t hash = 0x811c9dc5;
	uint8_t const * p = (uint8_t const *)llsd->value.string_.str;
	CHECK_RET_MSG( llsd->type_ == LLSD_STRING, 0, "map key hashing function received non-string\n" );
	while ( (*p) != '\0' )
	{
		hash *= FNV_PRIME;
		hash ^= *p++;
	}
	return hash;
}


static int key_eq(void const * const l, void const * const r)
{
	llsd_t * llsd_l = (llsd_t *)l;
	llsd_t * llsd_r = (llsd_t *)r;
	CHECK_RET_MSG( llsd_l->type_ == LLSD_STRING, -1, "map key compare function received non-string as left side\n" );
	CHECK_RET_MSG( llsd_r->type_ == LLSD_STRING, -1, "map key compare function received non-string as right side\n" );
	return (strcmp( llsd_l->value.string_.str, llsd_r->value.string_.str ) == 0);
}


static void llsd_initialize( llsd_t * llsd, llsd_type_t type_, ... )
{
	va_list args;
	uint8_t * p;

	CHECK_PTR( llsd );
	
	llsd->type_ = type_;

	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			break;

		case LLSD_BOOLEAN:
			va_start( args, type_ );
			llsd->value.bool_ = va_arg( args, int );
			va_end(args);
			break;

		case LLSD_INTEGER:
			va_start( args, type_ );
			llsd->value.int_ = va_arg(args, int );
			va_end(args);
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			memcpy( llsd->value.uuid_.bits, va_arg( args, uint8_t* ), UUID_LEN );
			va_end( args );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			llsd->value.date_ = va_arg( args, double );
			va_end( args );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			llsd->value.string_.dyn = TRUE;
			p = va_arg( args, uint8_t* );
			llsd->value.string_.str = strdup( p );
			llsd->value.string_.escaped = va_arg( args, int );
			va_end( args );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			llsd->value.uri_.dyn = TRUE;
			p = va_arg( args, uint8_t* );
			llsd->value.uri_.uri = strdup( p );
			va_end( args );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			llsd->value.binary_.dyn = TRUE;
			llsd->value.binary_.size = va_arg( args, int );
			llsd->value.binary_.data = CALLOC( llsd->value.binary_.size, sizeof(uint8_t) );
			p = va_arg( args, uint8_t* );
			memcpy( llsd->value.binary_.data, p, llsd->value.binary_.size );
			va_end( args );
			break;
		case LLSD_ARRAY:
			array_initialize( &(llsd->value.array_.array), &llsd_delete );
			break;
		case LLSD_MAP:
			ht_initialize( &(llsd->value.map_.ht), DEFAULT_MAP_CAPACITY, &fnv_key_hash, &llsd_delete, &key_eq, &llsd_delete );
			break;
	}
}


static void llsd_deinitialize( llsd_t * llsd )
{
	CHECK_PTR( llsd );
	
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
		case LLSD_UUID:
		case LLSD_DATE:
			return;
		case LLSD_STRING:
			if ( llsd->value.string_.dyn )
				FREE( llsd->value.string_.str );
			break;
		case LLSD_URI:
			if ( llsd->value.uri_.dyn )
				FREE( llsd->value.uri_.uri );
			break;
		case LLSD_BINARY:
			if ( llsd->value.binary_.dyn )
				FREE( llsd->value.binary_.data );
			break;
		case LLSD_ARRAY:
			array_deinitialize( &llsd->value.array_.array );
			break;
		case LLSD_MAP:
			ht_deinitialize( &llsd->value.map_.ht );
			break;
	}
}


llsd_t * llsd_new( llsd_type_t type_, ... )
{
	va_list args;
	llsd_t * llsd;
	int a1;
	uint8_t * a2;
	double a3;
	
	/* allocate the llsd object */
	llsd = CALLOC(1, sizeof(llsd_t));
	CHECK_PTR_RET_MSG( llsd, NULL, "failed to heap allocate llsd object\n" );

	switch( type_ )
	{
		case LLSD_UNDEF:
			llsd_initialize( llsd, type_ );
			break;

		case LLSD_BOOLEAN:
		case LLSD_INTEGER:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			va_end( args );
			llsd_initialize( llsd, type_, a1 );
			break;

		case LLSD_DATE:
			va_start( args, type_ );
			a3 = va_arg( args, double );
			va_end( args );
			llsd_initialize( llsd, type_, a3 );
			break;

		case LLSD_UUID:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_STRING:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			a1 = va_arg( args, int );
			va_end( args );
			llsd_initialize( llsd, type_, a2, a1 );
			break;

		case LLSD_URI:
			va_start( args, type_ );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a2 );
			break;

		case LLSD_BINARY:
			va_start( args, type_ );
			a1 = va_arg( args, int );
			a2 = va_arg( args, uint8_t* );
			va_end( args );
			llsd_initialize( llsd, type_, a1, a2 );
			break;

		case LLSD_ARRAY:
		case LLSD_MAP:
			llsd_initialize( llsd, type_ );
			break;
	}

	return llsd;
}

void llsd_delete( void * p )
{
	llsd_t * llsd = (llsd_t *)p;
	CHECK_PTR( llsd );

	/* deinitialize it */
	llsd_deinitialize( llsd );

	FREE( llsd );
}

llsd_type_t llsd_get_type( llsd_t * llsd )
{
	CHECK_PTR_RET( llsd, LLSD_UNDEF );
	return llsd->type_;
}

int llsd_get_size( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( ((llsd->type_ == LLSD_ARRAY) || (llsd->type_ == LLSD_MAP)), -1, "getting size of non-container llsd object\n" );

	if ( llsd->type_ == LLSD_ARRAY )
		return array_size( &llsd->value.array_.array );
	
	return ht_size( &llsd->value.map_.ht );
}


llsd_bool_t llsd_as_bool( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, FALSE, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			return FALSE;
		case LLSD_BOOLEAN:
			return llsd->value.bool_;
		case LLSD_INTEGER:
			if ( llsd->value.int_ != 0 )
				return TRUE;
			return FALSE;
		case LLSD_REAL:
			if ( llsd->value.real_ != 0. )
				return TRUE;
			return FALSE;
		case LLSD_UUID:
			if ( memcmp( llsd->value.uuid_.bits, zero_uuid.bits, UUID_LEN ) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_STRING:
			if ( strlen(llsd->value.string_.str) == 0 )
				return FALSE;
			return TRUE;
		case LLSD_DATE:
			WARN( "illegal conversion of date to bool\n" );
			break;
		case LLSD_URI:
			WARN( "illegal conversion of uri to bool\n" );
			break;
		case LLSD_BINARY:
			if ( llsd->value.binary_.size == 0 )
				return FALSE;
			return TRUE;
		case LLSD_ARRAY:
			WARN( "illegal conversion of array to bool\n" );
			break;
		case LLSD_MAP:
			WARN( "illegal conversion of map to bool\n" );
			break;
	}
	return FALSE;
}

llsd_int_t llsd_as_int( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, 0, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
			return 0;
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return 0;
			return 1;
		case LLSD_INTEGER:
			return llsd->value.int_;
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
	return 0;
}

llsd_real_t llsd_as_real( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, FALSE, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return 0.;
			return 1.;
		case LLSD_INTEGER:
			return (llsd_real_t)llsd->value.int_;
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
}

llsd_uuid_t llsd_as_uuid( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, zero_uuid, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			WARN( "illegal conversion of bool to uuid\n" );
			break;
		case LLSD_INTEGER:
			WARN( "illegal conversion of int to uuid\n" );
			break;
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}

	return zero_uuid;
}

llsd_string_t llsd_as_string( llsd_t * llsd )
{
	static int8_t tmp[11];
	int len;

	CHECK_PTR_RET_MSG( llsd, false_string, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return false_string;
			return true_string;
		case LLSD_INTEGER:
			len = snprintf( tmp, 10, "%d", llsd->value.int_ );
			return (llsd_string_t){ .dyn = FALSE, .escaped = FALSE, .str = tmp };
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
}

llsd_date_t llsd_as_date( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, 0., "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			WARN( "illegal conversion of bool to uuid\n" );
			break;
		case LLSD_INTEGER:
			return (llsd_date_t)llsd->value.int_;
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
}

llsd_uri_t llsd_as_uri( llsd_t * llsd )
{
	CHECK_PTR_RET_MSG( llsd, empty_uri, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			WARN( "illegal conversion of bool to uri\n" );
			break;
		case LLSD_INTEGER:
			WARN( "illegal conversion of int to uri\n" );
			break;
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
}

llsd_binary_t llsd_as_binary( llsd_t * llsd )
{
	static uint8_t tmp[8];
	llsd_binary_t tmpbin;
	uint32_t tmpl;

	CHECK_PTR_RET_MSG( llsd, empty_binary, "invalid llsd pointer\n" );
	switch( llsd->type_ )
	{
		case LLSD_UNDEF:
		case LLSD_BOOLEAN:
			if ( llsd->value.bool_ == FALSE )
				return false_binary;
			return true_binary;
		case LLSD_INTEGER:
			tmpl = htonl( llsd->value.int_ );
			memcpy( tmp, &tmpl, sizeof(uint32_t) );
			return (llsd_binary_t){ .size = 4, .data = tmp };
		case LLSD_UUID:
		case LLSD_STRING:
		case LLSD_DATE:
		case LLSD_URI:
		case LLSD_BINARY:
		case LLSD_ARRAY:
		case LLSD_MAP:
			break;
	}
}

static llsd_t * llsd_reserve_binary( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size );
	llsd->type_ = LLSD_BINARY;
	llsd->value.binary_.dyn = TRUE;
	llsd->value.binary_.size = size;
	llsd->value.binary_.data = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_string( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size + 1 );
	llsd->type_ = LLSD_STRING;
	llsd->value.string_.dyn = TRUE;
	llsd->value.string_.escaped = FALSE;
	llsd->value.string_.str = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

static llsd_t * llsd_reserve_uri( uint32_t size )
{
	llsd_t * llsd = CALLOC( 1, sizeof(llsd_t) + size + 1 );
	llsd->type_ = LLSD_URI;
	llsd->value.uri_.dyn = TRUE;
	llsd->value.uri_.uri = (uint8_t*)( ((void*)llsd) + sizeof(llsd_t) );
	return llsd;
}

llsd_t * llsd_parse_binary( FILE * fin )
{
	int i;
	uint8_t p;
	uint32_t t1;
	double t2;
	uint16_t t3[UUID_LEN];
	llsd_t * llsd;
	llsd_t * key;

	CHECK_PTR_RET( fin, NULL );

	while( !feof( fin ) )
	{
		/* read the type marker */
		fread( &p, sizeof(uint8_t), 1, fin );

		switch( p )
		{
			case '!':
				return llsd_new( LLSD_UNDEF );
			case '1':
				return llsd_new( LLSD_BOOLEAN, TRUE );
			case '0':
				return llsd_new( LLSD_BOOLEAN, FALSE );
			case 'i':
				fread( &t1, sizeof(uint32_t), 1, fin );
				return llsd_new( LLSD_INTEGER, ntohl( t1 ) );
			case 'r':
				fread( &t2, sizeof(double), 1, fin );
				return llsd_new( LLSD_REAL, ntohd( t2 ) );
			case 'u':
				fread( t3, sizeof(uint8_t), UUID_LEN, fin );
				return llsd_new( LLSD_UUID, t3 );
			case 'b':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_binary( t1 );
				fread( &(llsd->value.bool_), sizeof(uint8_t), t1, fin );
				return llsd;
			case 's':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_string( t1 ); /* allocates t1 + 1 bytes */
				fread( llsd->value.string_.str, sizeof(uint8_t), t1 + 1, fin );
				/* TODO: detect if it is escaped and set the escaped flag */
				return llsd;
			case 'l':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_reserve_uri( t1 ); /* allocates t1 + 1 bytes */
				fread( llsd->value.uri_.uri, sizeof(uint8_t), t1 + 1, fin );
				return llsd;
			case 'd':
				fread( &t2, sizeof(double), 1, fin );
				t2 = ntohd( t2 );
				return llsd_new( LLSD_DATE, t2 );
			case '[':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_new_empty_array();
				for ( i = 0; i < t1; ++i )
				{
					/* parse and append the array member */
					llsd_array_append( llsd, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != ']' )
				{
					WARN( "array didn't end with ']'\n" );
				}
				return llsd;
			case '{':
				fread( &t1, sizeof(uint32_t), 1, fin );
				t1 = ntohl( t1 );
				llsd = llsd_new_empty_map();
				for ( i = 0; i < t1; ++i )
				{
					/* parse the key */
					key = llsd_parse_binary( fin );
					if ( llsd_get_type( key ) != LLSD_STRING )
					{
						WARN( "key is not an LLSD_STRING\n" );
					}

					/* parse and append the key/value pair */
					llsd_map_insert( llsd, key, llsd_parse_binary( fin ) );
				}
				fread( &p, sizeof(uint8_t), 1, fin );
				if ( p != '}' )
				{
					WARN( "map didn't end with '}'\n" );
				}
				return llsd;
		}
	}
}

llsd_t * llsd_parse_notation( FILE * fin )
{




}

llsd_t * llsd_parse_xml( FILE * fin )
{
}

#define SIG_LEN (18)
llsd_t * llsd_parse( FILE *fin )
{
	uint8_t sig[18];

	CHECK_RET_MSG( fin, NULL, "invalid file pointer\n" );

	fread( sig, sizeof(uint8_t), SIG_LEN, fin );
	if ( strncmp( sig, "<? LLSD/Binary ?>\n", SIG_LEN ) == 0 )
	{
		return llsd_parse_binary( fin );
	}
	else if ( strncmp( sig, "<?llsd/notation?>\n", SIG_LEN ) == 0 )
	{
		return llsd_parse_notation( fin );
	}
	else
	{
		rewind( fin );
		return llsd_parse_xml( fin );
	}
}

void llsd_format_xml( llsd_t * llsd, FILE * fout )
{
}

void llsd_format_notation( llsd_t * llsd, FILE * fout )
{
}

void llsd_format_binary( llsd_t * llsd, FILE * fout )
{
}

void llsd_format( llsd_t * llsd, llsd_serializer_t fmt, FILE * fout )
{
	CHECK_PTR( llsd );
	CHECK_PTR( fout );

	switch ( fmt )
	{
		case LLSD_ENC_XML:
			llsd_format_xml( llsd, fout );
			break;
		case LLSD_ENC_NOTATION:
			llsd_format_notation( llsd, fout );
			break;
		case LLSD_ENC_BINARY:
			llsd_format_binary( llsd, fout );
			break;
		case LLSD_ENC_JSON:
			WARN( "JSON encoding not supported yet\n" );
	}
}

