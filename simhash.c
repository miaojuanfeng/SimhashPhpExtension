/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_simhash.h"

static int le_simhash;

#define SIMHASH_BIT 64
zend_class_entry *simhash_ce;

/**
* Get the number of bit which is 1
*
* @param 	 long   $fingerprint
*
* @return  	 int   
*
* public function hamming($fingerprint)
*/
PHP_METHOD(simhash, hamming)
{
	zend_ulong binary;
	unsigned int tmp;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &binary) == FAILURE) {
		return;
	}

	tmp = binary - ((binary >>1) &033333333333) - ((binary >>2) &011111111111);
    RETURN_LONG( ((tmp + (tmp >>3)) &030707070707) %63 );
}
/**
* Compare the difference between Two hash value
*
* @param 	 long   $hash1
* @param 	 long   $hash2
*
* @return  	 long   $hash1 ^ $hash2 that means fingerprint
*
* public function compare($hash1, $hash2)
*/
PHP_METHOD(simhash, compare)
{
	zend_ulong hash1;
	zend_ulong hash2;
	zend_ulong result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &hash1, &hash2) == FAILURE) {
		return;
	}

	result = hash1 ^ hash2;
	RETURN_LONG(result);
}
/**
* Generate a fingerprint according to Hash array
*
* @param 	 array   $hash_array
*
* @return  	 long
*
* public function binary($hash_array)
*/
PHP_METHOD(simhash, binary)
{
	zval *arr;
	HashPosition pos;
	zval **value;

	float hash_vector[SIMHASH_BIT];
	zend_ulong token_hash = 0;
	zend_ulong simhash = 0;
	int current_bit = 0;

	int i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &arr) == FAILURE) {
		return;
	}

	memset(hash_vector, 0, SIMHASH_BIT * sizeof(float));

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);
	    zend_hash_has_more_elements_ex(Z_ARRVAL_P(arr), &pos) == SUCCESS;
	    zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos)){
		if(zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void**)&value, &pos) == FAILURE){
			continue;
		}
		if(Z_TYPE_PP(value) == IS_LONG){
			int j;
			token_hash = Z_LVAL_PP(value);
			for(j=SIMHASH_BIT-1; j>=0; j--) {
	            current_bit = token_hash & 0x1;
	            if(current_bit == 1) {
	                hash_vector[j] += 1;
	            } else {
	                hash_vector[j] -= 1;
	            }
	            token_hash = token_hash >> 1;
	        }
		}
	}

	for(i=0; i<SIMHASH_BIT; i++) {
        if(hash_vector[i] > 0) {
            simhash = (simhash << 1) + 0x1;
        } else {
            simhash = simhash << 1;
        }
    }

    RETURN_LONG(simhash);
}
/**
* Hash a Text Keyword to a long number
*
* @param 	 array   $keyword
*
* @return    array   Array with every hashed item
*
* public function hash($keyword)
*/
PHP_METHOD(simhash, hash)
{
	zval *arr;
	HashPosition pos;
	zval **value;
	zval *retval;

	zend_ulong token_hash = 0;
	zval *hash_item;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &arr) == FAILURE) {
		return;
	}

	MAKE_STD_ZVAL(retval);
	array_init(retval);

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);
	    zend_hash_has_more_elements_ex(Z_ARRVAL_P(arr), &pos) == SUCCESS;
	    zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos)){
		if(zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void**)&value, &pos) == FAILURE){
			continue;
		}
		if(Z_TYPE_PP(value) == IS_STRING){
			token_hash = zend_hash_func(Z_STRVAL_PP(value), Z_STRLEN_PP(value));
			MAKE_STD_ZVAL(hash_item);
			ZVAL_LONG(hash_item, token_hash);
			zend_hash_next_index_insert(Z_ARRVAL_P(retval), &hash_item, sizeof(zval*), NULL);
		}
	}
	RETURN_ZVAL(retval, 0, 1);
}
/**
* Hash a Text Keyword with Weigth to a long number
*
* @param 	 array   $keyword
*
* @return    long    $fingerprint
*
* public function sign($keyword)
*/
PHP_METHOD(simhash, sign)
{
	zval *arr;
	HashPosition pos;
	char *key;
	uint key_len;
	zend_ulong idx;
	zval **value;
	char key_type;

	float hash_vector[SIMHASH_BIT];
	zend_ulong token_hash = 0;
	zend_ulong simhash = 0;
	int current_bit = 0;

	int i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &arr) == FAILURE) {
		return;
	}

	memset(hash_vector, 0, SIMHASH_BIT * sizeof(float));

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);
	    zend_hash_has_more_elements_ex(Z_ARRVAL_P(arr), &pos) == SUCCESS;
	    zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos)){
		if( (key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(arr), &key, &key_len, &idx, 0, &pos)) == FAILURE ){
			php_error(E_ERROR, "Array key should be String or Number");
		}
		if(	zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void**)&value, &pos) == FAILURE || ( Z_TYPE_PP(value) != IS_LONG && Z_TYPE_PP(value) != IS_DOUBLE) ){
			php_error(E_ERROR, "Array Value should be Number");
		}
		if( Z_TYPE_PP(value) == IS_LONG ){
			convert_to_double(*value);
		}
		if(Z_TYPE_PP(value) == IS_DOUBLE){
			int j;
			if( key_type == HASH_KEY_IS_STRING ){
				token_hash = zend_hash_func(key, key_len);
			}else{
				token_hash = idx;
			}
			for(j=SIMHASH_BIT-1; j>=0; j--) {
	            current_bit = token_hash & 0x1;
	            if(current_bit == 1) {
	                hash_vector[j] += Z_DVAL_PP(value);
	            } else {
	                hash_vector[j] -= Z_DVAL_PP(value);
	            }
	            token_hash = token_hash >> 1;
	        }
		}
	}

	for(i=0; i<SIMHASH_BIT; i++) {
        if(hash_vector[i] > 0) {
            simhash = (simhash << 1) + 0x1;
        } else {
            simhash = simhash << 1;
        }
    }

    RETURN_LONG(simhash);
}

zend_function_entry simhash_methods[] = {
	PHP_ME(simhash, hash, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, binary, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, compare, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, hamming, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, sign, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};


PHP_MINIT_FUNCTION(simhash)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Simhash", simhash_methods);
	simhash_ce = zend_register_internal_class(&ce TSRMLS_CC);
	
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(simhash)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(simhash)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(simhash)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(simhash)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "simhash support", "enabled");
	php_info_print_table_end();
}

const zend_function_entry simhash_functions[] = {
	PHP_FE_END	
};

zend_module_entry simhash_module_entry = {
	STANDARD_MODULE_HEADER,
	"simhash",
	simhash_functions,
	PHP_MINIT(simhash),
	PHP_MSHUTDOWN(simhash),
	PHP_RINIT(simhash),
	PHP_RSHUTDOWN(simhash),
	PHP_MINFO(simhash),
	PHP_SIMHASH_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SIMHASH
ZEND_GET_MODULE(simhash)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
