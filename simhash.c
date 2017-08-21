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

/* If you declare any globals in php_simhash.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(simhash)
*/

/* True global resources - no need for thread safety here */
static int le_simhash;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("simhash.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_simhash_globals, simhash_globals)
    STD_PHP_INI_ENTRY("simhash.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_simhash_globals, simhash_globals)
PHP_INI_END()
*/
/* }}} */

#define SIMHASH_BIT 64
zend_class_entry *simhash_ce;

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_simhash_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_simhash_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "simhash", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_simhash_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_simhash_init_globals(zend_simhash_globals *simhash_globals)
{
	simhash_globals->global_value = 0;
	simhash_globals->global_string = NULL;
}
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

PHP_METHOD(simhash, getBinary)
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

PHP_METHOD(simhash, sign)
{
	zval *arr;
	HashPosition pos;
	char *key;
	uint key_len;
	uint idx;
	zval **value;
	zval *retval;

	float hash_vector[SIMHASH_BIT];
	zend_ulong token_hash = 0;
	zend_ulong simhash = 0;
	int current_bit = 0;

	int i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &arr) == FAILURE) {
		return;
	}

	MAKE_STD_ZVAL(retval);
	array_init(retval);

	memset(hash_vector, 0, SIMHASH_BIT * sizeof(float));

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);
	    zend_hash_has_more_elements_ex(Z_ARRVAL_P(arr), &pos) == SUCCESS;
	    zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos)){
		if(zend_hash_get_current_key_ex(Z_ARRVAL_P(arr), &key, &key_len, &idx, 0, &pos) != HASH_KEY_IS_STRING){
			php_error(E_ERROR, "Array key should be String");
		}
		if(zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void**)&value, &pos) == FAILURE || Z_TYPE_PP(value) != IS_LONG ){
			php_error(E_ERROR, "Array Value should be Long int");
		}
		if(Z_TYPE_PP(value) == IS_LONG){
			int j;
			token_hash = zend_hash_func(key, key_len);
			for(j=SIMHASH_BIT-1; j>=0; j--) {
	            current_bit = token_hash & 0x1;
	            if(current_bit == 1) {
	                hash_vector[j] += Z_LVAL_PP(value);
	            } else {
	                hash_vector[j] -= Z_LVAL_PP(value);
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
/* }}} */
zend_function_entry simhash_methods[] = {
	PHP_ME(simhash, hash, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, getBinary, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, compare, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, hamming, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(simhash, sign, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(simhash)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Simhash", simhash_methods);
	simhash_ce = zend_register_internal_class(&ce TSRMLS_CC);
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(simhash)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(simhash)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(simhash)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(simhash)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "simhash support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ simhash_functions[]
 *
 * Every user visible function must have an entry in simhash_functions[].
 */
const zend_function_entry simhash_functions[] = {
	PHP_FE(confirm_simhash_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in simhash_functions[] */
};
/* }}} */

/* {{{ simhash_module_entry
 */
zend_module_entry simhash_module_entry = {
	STANDARD_MODULE_HEADER,
	"simhash",
	simhash_functions,
	PHP_MINIT(simhash),
	PHP_MSHUTDOWN(simhash),
	PHP_RINIT(simhash),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(simhash),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(simhash),
	PHP_SIMHASH_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

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
