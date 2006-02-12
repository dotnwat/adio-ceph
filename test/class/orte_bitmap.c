/* 
 * Testcase for ORTE bitmap
 */

#include "orte_config.h"
#include "orte/orte_constants.h"

#include <stdio.h>
#include "support.h"
#include "orte/class/orte_bitmap.h"
#include "opal/runtime/opal.h"

#define STANDALONE

#define BSIZE 26
#define SIZE_OF_CHAR (sizeof(char) * 8)
#define ORTE_INVALID_BIT -1
#define ERR_CODE -2

#define PRINT_VALID_ERR \
    fprintf(error_out, "================================ \n"); \
    fprintf(error_out, "This is supposed to throw error \n"); \
    fprintf(error_out, "================================ \n")

static void test_bitmap_set(orte_bitmap_t *bm);
static void test_bitmap_clear(orte_bitmap_t *bm);
static void test_bitmap_is_set(orte_bitmap_t *bm);
static void test_bitmap_clear_all(orte_bitmap_t *bm);
static void test_bitmap_set_all(orte_bitmap_t *bm);
static void test_bitmap_find_and_set(orte_bitmap_t *bm);
static void test_bitmap_find_size(orte_bitmap_t *bm);


static int set_bit(orte_bitmap_t *bm, int bit);
static int clear_bit(orte_bitmap_t *bm, int bit);
static int is_set_bit(orte_bitmap_t *bm, int bit);
static int clear_all(orte_bitmap_t *bm);
static int set_all(orte_bitmap_t *bm);
static int find_and_set(orte_bitmap_t *bm, int bit);
static int find_size(orte_bitmap_t *bm);

#define WANT_PRINT_BITMAP 0
#if WANT_PRINT_BITMAP
static void print_bitmap(orte_bitmap_t *bm);
#endif

static FILE *error_out=NULL;

int main(int argc, char *argv[])
{
    /* Local variables */
    orte_bitmap_t bm;
    int err;
    
    opal_init();

    /* Perform overall test initialization */
    test_init("orte_bitmap_t");

#ifdef STANDALONE
    error_out = stderr;
#else
    error_out = fopen( "./orte_bitmap_test_out.txt", "w" );
    if( error_out == NULL ) error_out = stderr;
#endif
    /* Initialize bitmap  */

    OBJ_CONSTRUCT(&bm, orte_bitmap_t);

    PRINT_VALID_ERR;
    OBJ_CONSTRUCT(&bm, orte_bitmap_t);
    
    fprintf(error_out, "\nTesting bitmap set... \n");
    test_bitmap_set(&bm);

    fprintf(error_out, "\nTesting bitmap clear ... \n");
    test_bitmap_clear(&bm);

    fprintf(error_out, "\nTesting bitmap is_set ... \n");
    test_bitmap_is_set(&bm);

    fprintf(error_out, "\nTesting bitmap clear_all... \n");
    test_bitmap_clear_all(&bm);

    fprintf(error_out, "\nTesting bitmap set_all... \n");
    test_bitmap_set_all(&bm);

    fprintf(error_out, "\nTesting bitmap find_and_set... \n");
    test_bitmap_find_and_set(&bm);

    fprintf(error_out, "\nTesting bitmap find_size... \n");
    test_bitmap_find_size(&bm);

    OBJ_DESTRUCT(&bm);

    fprintf(error_out, "\n~~~~~~     Testing complete     ~~~~~~ \n\n");

    test_finalize();
#ifndef STANDALONE
    fclose(error_out);
#endif

    opal_finalize();

    return 0;
}



void test_bitmap_set(orte_bitmap_t *bm) {

    /* start of bitmap and boundaries */
    set_bit(bm, 0);
    set_bit(bm, 1);
    set_bit(bm, 7);
    set_bit(bm, 8);
    /* middle of bitmap  */
    set_bit(bm, 24);

    /* end of bitmap initial size */
    set_bit(bm, 31);
    set_bit(bm, 32);
    
    /* beyond bitmap -- this is valid */
    set_bit(bm, 44);
    set_bit(bm, 82);

}


void test_bitmap_clear(orte_bitmap_t *bm) {

    /* Valid set bits  */
    clear_bit(bm, 29);
    clear_bit(bm, 31);
    clear_bit(bm, 33);
    clear_bit(bm, 32);
    clear_bit(bm, 0);
    /* check auto-extend */
    clear_bit(bm, 142);
    clear_bit(bm, 1024);
}


void test_bitmap_is_set(orte_bitmap_t *bm)
{
    int result=0;

    /* First set some bits */
    test_bitmap_set(bm);
    is_set_bit(bm, 0);
    is_set_bit(bm, 1);
    is_set_bit(bm, 31);
    is_set_bit(bm, 32);

    PRINT_VALID_ERR;
    result = is_set_bit(bm, 11122);
    TEST_AND_REPORT(result,ERR_CODE,"orte_bitmap_is_set_bit");    
}


void test_bitmap_find_and_set(orte_bitmap_t *bm) 
{
    int bsize;
    int result=0;

    orte_bitmap_clear_all_bits(bm);
    result = find_and_set(bm, 0);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
    result = find_and_set(bm, 1);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
    result = find_and_set(bm, 2);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
    result = find_and_set(bm, 3);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    

    result = orte_bitmap_set_bit(bm, 5);
    result = find_and_set(bm, 4);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
    
    result = orte_bitmap_set_bit(bm, 6);
    result = orte_bitmap_set_bit(bm, 7);

    /* Setting beyond a char boundary */
    result = find_and_set(bm, 8);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
    orte_bitmap_set_bit(bm, 9);
    result = find_and_set(bm, 10);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    

    /* Setting beyond the current size of bitmap  */
    orte_bitmap_set_all_bits(bm);
    bsize = bm->array_size * SIZE_OF_CHAR;
    result = find_and_set(bm, bsize);
    TEST_AND_REPORT(result, 0, "orte_bitmap_find_and_set_first_unset_bit");    
}

void test_bitmap_clear_all(orte_bitmap_t *bm)
{
    int result = clear_all(bm);
    TEST_AND_REPORT(result, 0, " error in orte_bitmap_clear_all_bits");
}


void test_bitmap_set_all(orte_bitmap_t *bm)
{
    int result = set_all(bm);
    TEST_AND_REPORT(result, 0, " error in orte_bitmap_set_ala_bitsl");
}

void test_bitmap_find_size(orte_bitmap_t *bm)
{
    int result = find_size(bm);
    TEST_AND_REPORT(result, 0, " error in orte_bitmap_size");
}


int set_bit(orte_bitmap_t *bm, int bit)
{
    int err = orte_bitmap_set_bit(bm, bit);
    if (err != 0 
	|| !(bm->bitmap[bit/SIZE_OF_CHAR] & (1 << bit % SIZE_OF_CHAR))) {
	    fprintf(error_out, "ERROR: set_bit for bit = %d\n\n", bit);
	    return ERR_CODE;
	}
    return 0;
}


int clear_bit(orte_bitmap_t *bm, int bit)
{
    int err = orte_bitmap_clear_bit(bm, bit);
    if ((err != 0)
	|| (bm->bitmap[bit/SIZE_OF_CHAR] & (1 << bit % SIZE_OF_CHAR))) {
	fprintf(error_out, "ERROR: clear_bit for bit = %d \n\n", bit);
	return ERR_CODE;
    }

    return 0;
}


int is_set_bit(orte_bitmap_t *bm, int bit) 
{
    int result = orte_bitmap_is_set_bit(bm, bit);
    if (((1 == result) 
	&& !(bm->bitmap[bit/SIZE_OF_CHAR] & (1 << bit % SIZE_OF_CHAR)))
	|| (result < 0)
	|| ((0 == result) 
	    &&(bm->bitmap[bit/SIZE_OF_CHAR] & (1 << bit % SIZE_OF_CHAR)))) {
	fprintf(error_out, "ERROR: is_set_bit for bit = %d \n\n",bit);
	return ERR_CODE;
    }
	
    return 0;
}


int find_and_set(orte_bitmap_t *bm, int bit) 
{
    int ret;
    size_t pos;
    /* bit here is the bit that should be found and set, in the top
       level stub, this function will be called in sequence to test */

    ret = orte_bitmap_find_and_set_first_unset_bit(bm, &pos);
    if (ret != ORTE_SUCCESS) return ret;

    if ((int)pos != bit) {
	fprintf(error_out, "ERROR: find_and_set: expected to find_and_set %d\n\n",
		bit);
	return ERR_CODE;
    }

    return 0;
}


int clear_all(orte_bitmap_t *bm) 
{
    int i, err;
    err = orte_bitmap_clear_all_bits(bm);
    for (i = 0; i < (int)bm->array_size; ++i)
	if (bm->bitmap[i] != 0) {
	    fprintf(error_out, "ERROR: clear_all for bitmap arry entry %d\n\n",
		    i);
	    return ERR_CODE;
	}
    return 0;
}
	    

int set_all(orte_bitmap_t *bm)
{
   int i, err;
   err = orte_bitmap_set_all_bits(bm);
   for (i = 0; i < (int)bm->array_size; ++i)
       if (bm->bitmap[i] != 0xff) {
	   fprintf(error_out, "ERROR: set_all for bitmap arry entry %d\n\n", i);
	   return ERR_CODE;
       }
   return 0;
}

	
int find_size(orte_bitmap_t *bm)
{
    if (orte_bitmap_size(bm) != (int)bm->legal_numbits) {
	fprintf(error_out, "ERROR: find_size: expected %d reported %d\n\n",
		(int) bm->array_size, (int) orte_bitmap_size(bm));
        
	return ERR_CODE;
    }
    return 0;
}


#if WANT_PRINT_BITMAP
void print_bitmap(orte_bitmap_t *bm) 
{
    /* Accessing the fields within the structure, since its not an
       opaque structure  */

    int i;
    for (i = 0; i < (int)bm->array_size; ++i) {
	fprintf(error_out, "---\n bitmap[%d] = %x \n---\n\n", i, 
		(bm->bitmap[i] & 0xff));
    }
    fprintf(error_out, "========================= \n");
    return;
}
#endif
