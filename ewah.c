/*RLE-WAH  implementation based on "An Efficient Compression Scheme for Bitmap Indices"
 developed by Kesheng Wu, Ekow J. Otoo and Arie Shoshani, http://escholarship.org/uc/item/2sp907t5#page-1
Written for use with BZET-API by Robert Uzgalis
Author: Stefan Wojciechowski

This code can run 8,16,32,64 bit Word versions of WAH. Define ALIGN_8,ALIGN_16, ALIGN_32, ALIGN_64 respectively.
It is endian agnostic*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
void *__gxx_personality_v0;

#define ALIGN_32
#if !defined(ALIGN_8) && !defined(ALIGN_16) && !defined(ALIGN_32) && !defined(ALIGN_64)
#define ALIGN_32
#endif


#if defined(ALIGN_8)
#define align_t unsigned char
#elif defined(ALIGN_16)
#define align_t unsigned short
#elif defined(ALIGN_32)
#define align_t unsigned
#elif defined(ALIGN_64)
#define align_t unsigned long long

#else
#error one of symbols ALIGN_8, ALIGN_16, ALIGN_32, ALIGN_64 must be defined
#endif

#define align_bit_size (align_t) sizeof(align_t)*8
#define WORD_COUNT_MASK (~((align_t)3 << (align_bit_size-2)))
#define FILL_ZERO  ((align_t)2 << (align_bit_size-2))
#define FILL_ZERO_MAX (FILL_ZERO + WORD_COUNT_MASK)
#define FILL_ONE ((align_t)3 << (align_t)(align_bit_size-2))
#define FILL_ONE_MAX   (FILL_ONE + WORD_COUNT_MASK)
#define FILL_ONES  ~((align_t)2 << (align_bit_size-2))
#define FILL_ZEROS 0
#define COUNTMASK 1
#define WORDBYTES sizeof(align_t)

struct vector {					
	unsigned size;
	unsigned allocSize;
	align_t *data;				
};
struct activeWord {
		align_t value;
		unsigned nbits;
};
struct bitVector {
	struct activeWord active;
	struct vector vec;
};
typedef struct bitVector bitVector;
typedef struct vector vector;
typedef struct activeWord activeWord;
#define PUSH( vect, value ) growIfNeeded(&vect), vect.data[vect.size++]=value
#define EMPTY( vect ) vect.size == 0
#define BACK( vect ) vect.data[vect.size-1]  
/*#define INIT( vector , vectorSize )\
	vector = (struct bitVector *) malloc(sizeof(bitVector)),\
	vector->vec.data = (align_t *) malloc(vectorSize *sizeof(align_t)),\
	memset(vector->vec.data, 0, vectorSize*sizeof(align_t)),\
	vector->active.nbits = 0, \
	vector->active.value = 0,\
	vector->vec.size = 0 ,\
	vector->vec.allocSize = vectorSize;*/
#define FREE( vector ) \
	free(vector->vec.data),\
	free(vector);
#define MIN( value1, value2 ) ((value1)<(value2) ? (value1) : (value2))
 bitVector * INIT(unsigned vectorSize)
{
	 bitVector *vector = (bitVector*) malloc(sizeof(bitVector));
	vector->vec.data = (align_t *) malloc(vectorSize *sizeof(align_t));
	memset(vector->vec.data, 0, vectorSize*sizeof(align_t));
	vector->active.nbits = 0;
	vector->active.value = 0;
	vector->vec.size = 0 ;
	vector->vec.allocSize = vectorSize;
	return vector;
}
int empty( vector* v){
	return (v->size == 0 ? 1 : 0);
}
void growIfNeeded( vector * v){
	if(v->size >= v->allocSize){
			unsigned newSize = sizeof(align_t)* v->allocSize*2;			
			void * newMemory = realloc(v->data, newSize );
			assert(newMemory != NULL);
			v->data = (align_t *) newMemory;
			v->allocSize *= 2;
		}																	
}


void appendLiteral( bitVector* bv) { 															
	assert(bv->vec.size < bv->vec.allocSize);										
	if(EMPTY(bv->vec))																																											
		PUSH(bv->vec,bv->active.value);																
	else if (bv->active.value == 0 )	{															
		if(BACK(bv->vec)==0)																		
			BACK(bv->vec) = (FILL_ZERO + 2);																			
		else if (BACK(bv->vec) >=(align_t) FILL_ZERO && BACK(bv->vec) <(align_t) FILL_ZERO_MAX)							
			++BACK(bv->vec);																		
		else																						
			PUSH(bv->vec,bv->active.value);															 
	}																							
	else if(bv->active.value == (align_t)FILL_ONES ){																																							
		if(BACK(bv->vec) == bv->active.value)																													
				BACK(bv->vec) = FILL_ONE + 2;															 																	
		else if(BACK(bv->vec) >=(align_t) FILL_ONE && BACK(bv->vec) <(align_t) FILL_ONE_MAX)														
				++BACK(bv->vec);																					
		else																						
			PUSH(bv->vec,bv->active.value);															
	}else																						
		PUSH(bv->vec,bv->active.value);																
}																									
 void appendNewFills(bitVector* bv, unsigned long long n, align_t fillBase){
	while( n > 0)
	{
		unsigned long long will_append = MIN((align_t)WORD_COUNT_MASK, n);
		PUSH(bv->vec, fillBase + (align_t) will_append);
		n-=will_append;											
	}
}
void appendFill(bitVector* bv, unsigned long long n, align_t fillBit){								
	assert( bv->active.nbits == 0);
	assert(n>0);																					
	if(n>1 && !EMPTY(bv->vec)){																		
		if (fillBit == 0){																			
			if(BACK(bv->vec) >=(align_t) FILL_ZERO && BACK(bv->vec) <(align_t) FILL_ZERO_MAX){							
				align_t could_append = (align_t) WORD_COUNT_MASK - (BACK(bv->vec) & (align_t) WORD_COUNT_MASK);
				align_t will_append = (align_t) MIN(could_append, n);
				BACK(bv->vec) += will_append;
				n -= will_append;
			}
			appendNewFills(bv,n,FILL_ZERO);
		}else{ 
			if( BACK(bv->vec) >=(align_t) FILL_ONE && BACK(bv->vec) <(align_t) FILL_ONE_MAX){	
				align_t could_append = (align_t) WORD_COUNT_MASK - (BACK(bv->vec) & (align_t) WORD_COUNT_MASK);													
				align_t will_append = (align_t) MIN(could_append, n);
				BACK(bv->vec) += will_append;
				n-= will_append;
			}							
			appendNewFills(bv,n,FILL_ONE);
		}
	}else if(EMPTY(bv->vec)){																		
		if(fillBit == 0)																			
			appendNewFills(bv,n,FILL_ZERO);															 
		else																						
			appendNewFills(bv,n,FILL_ONE);														 
	}else{
																									
		bv->active.value = (fillBit?(align_t)FILL_ONES:0);													
		growIfNeeded(&bv->vec);
		appendLiteral(bv);																			
	}
}
																									
struct run{																							
const align_t * it;														
align_t fill;																
align_t nWords;																
align_t isFill;																			
};																								
typedef struct run run;

void runInit( run * r){r->it=0; r->fill=0; r->nWords=0; r->isFill=0; }						

void runDecode( run *r){																		
	if( *(r->it) > (align_t) FILL_ONES){																		
		r->fill = (*(r->it)>=FILL_ONE?(align_t)FILL_ONES:0);												 
		r->nWords = *(r->it) & (align_t) WORD_COUNT_MASK;															
		r->isFill = 1;																				
	}else{																						
		r->nWords = 1;																				 
		r->isFill = 0;																				
	}
}

align_t andOper(align_t x, align_t y){return x&y;}
align_t orOper(align_t x, align_t y){return x|y;}
align_t xorOper(align_t x, align_t y){return x^y;}
void generic_op( bitVector * x, bitVector *y, bitVector *z, align_t oper(align_t, align_t)){				
																																
																																
	 run xrun, yrun; runInit(&xrun); runInit(&yrun);																		
	xrun.it = x->vec.data; /*runDecode(&xrun);*/																				
	yrun.it = y->vec.data; /*runDecode(&yrun);*/																				
	while(xrun.it < x->vec.data + x->vec.size && yrun.it <y->vec.data + y->vec.size)	{										
		if(xrun.nWords == 0) /*++xrun.it,*/ runDecode(&xrun);																	
		if(yrun.nWords == 0) /*++yrun.it,*/ runDecode(&yrun);																	
		if(xrun.isFill){																										
			if(yrun.isFill){																									
				align_t nWords = MIN(xrun.nWords,yrun.nWords);																
				appendFill(z,nWords, oper(xrun.fill, yrun.fill));							
							
				xrun.nWords -= nWords, yrun.nWords -= nWords;																	
			}else{
				z->active.value = oper(xrun.fill, *(yrun.it));																																											
				growIfNeeded(&z->vec);
				appendLiteral(z);
				--yrun.nWords;																				
				--xrun.nWords;																						
			}
		}else if (yrun.isFill){																						
			z->active.value = oper(yrun.fill , *(xrun.it));																		
			growIfNeeded(&z->vec);
			appendLiteral(z);																									
			--yrun.nWords;
			--xrun.nWords; 
																																		
		}else{																													
			z->active.value = oper(*(xrun.it) , *(yrun.it));													
			growIfNeeded(&z->vec);
			appendLiteral(z);
			--yrun.nWords;
			--xrun.nWords; 
		}
		if(xrun.nWords == 0) ++xrun.it;/*, runDecode(&xrun);*/												
		if(yrun.nWords == 0) ++yrun.it;/*, runDecode(&yrun);*/	
	}										
			z->active.value = oper(x->active.value, y->active.value);															
			z->active.nbits = x->active.nbits;																					
} bitVector * create(){
	 bitVector * bv = INIT(10);
	return bv;
}
 void and_op( bitVector * x,  bitVector *y,  bitVector *z){
generic_op(x,y,z,andOper);
}
 void or_op( bitVector * x, bitVector *y,  bitVector *z){
generic_op(x,y,z,orOper);
}
 void xor_op( bitVector * x, bitVector *y,  bitVector *z){
generic_op(x,y,z,xorOper);
}
  bitVector * and_op_wrapper( bitVector *x,  bitVector *y){
	bitVector *z = create();
	and_op(x,y,z);
	return z;
}
  bitVector * or_op_wrapper( bitVector *x,  bitVector *y){
	bitVector *z = create();
	or_op(x,y,z);
	return z;
}
  bitVector * xor_op_wrapper( bitVector *x,  bitVector *y){
	bitVector *z = create();
	xor_op(x,y,z);
	return z;
}
 unsigned nWords(  bitVector * v){
	unsigned totalNumWords = 0;
	 run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){																											
		runDecode(&xrun);
		totalNumWords += xrun.nWords;
		xrun.it++;
	}
	return totalNumWords;
}
 void invert( bitVector * x){
	 bitVector * mask = INIT(x->vec.allocSize); 
	 bitVector * z = INIT(x->vec.allocSize); 
	unsigned xNumWords = nWords(x);

	appendFill(mask,xNumWords,(align_t)FILL_ONES);							
	xor_op(x,mask,z);														
	free(x->vec.data);														
	x->vec.data = z->vec.data;												
	free(z); FREE(mask);													
}
  bitVector * RLEnot( bitVector *x){
	 bitVector * mask = INIT(x->vec.allocSize); 
	 bitVector * z = INIT(x->vec.allocSize); 
	unsigned xNumWords = nWords(x);

	appendFill(mask,xNumWords,(align_t)FILL_ONES);									
	xor_op(x,mask,z);															
	return z;
}
void add_n_bits(bitVector *bv, unsigned count, unsigned value)
{
	printf("in add bits, count is %d\n",count);
	for (unsigned i = 0; i < count+1; i++)
	{
		bv->active.value |= value << bv->active.nbits++;
		if(bv->active.nbits == align_bit_size-1){
			appendLiteral(bv);
			bv->active.nbits = 0;
			bv->active.value = 0;
		}
	}
}
/*struct  bitVector * range (unsigned from, unsigned to, unsigned stride){
	struct bitVector *v = INIT(10);
	if(from>0)
		add_n_bits(v,from-1,0);
	if(stride > 0){
		unsigned i=from;
		while(1) //find out if equal or less than equal
		{
			add_n_bits(v,1,1);
			if(i++ >= to)
				break;
			add_n_bits(v,stride-1,0);
			i+=stride;
			if(i += stride-1)
				break;
		}
	}
	return v;
}*/
 bitVector * range(unsigned from, unsigned to){
	 bitVector *v = INIT(10);
	if(from>0)
		add_n_bits(v,from-1,0);
	add_n_bits(v,to-from,1);
	appendLiteral(v);
	v->active.nbits = 0;
	v->active.value = 0;
	return v;
}
void addBit( bitVector * v, unsigned i, unsigned inv){
	unsigned numFillOfZeros = i / (align_bit_size-1);									
	unsigned literalIndex = i % (align_bit_size-1);										
	align_t literalValue = 1;
	align_t fillBit = 0;
	literalValue = literalValue<<literalIndex;								
	if(inv){
		literalValue ^= (align_t)FILL_ONES;
		fillBit = (align_t)FILL_ONES;
	}
	if(numFillOfZeros > 0) appendFill(v, numFillOfZeros,fillBit);			
	
	v->active.value = literalValue; v->active.nbits = align_bit_size-1;
	appendLiteral(v);														
	v->active.value = 0; v->active.nbits = 0;

}
 void align( bitVector *x, bitVector *y,unsigned inv){
	int xNumWords = nWords(x);
	int yNumWords = nWords(y);												
	align_t fillBit = 0;
	if(inv)
		fillBit= (align_t)FILL_ONES;
	int appendSize = abs(xNumWords - yNumWords);
	if(xNumWords>yNumWords) appendFill(y,appendSize,fillBit);				
	else if(yNumWords>xNumWords) appendFill(x,appendSize,fillBit);
}
 void set( bitVector * x, unsigned i)
{
	 bitVector * temp = INIT(x->vec.allocSize);										
	 bitVector * z = INIT(x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){														
		or_op(x,temp,z);													
		free(x->vec.data);			 										
		x->vec.data = z->vec.data;											
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												
	}
	else{																	
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
 void unset( bitVector * x, unsigned i){
	 bitVector * temp = INIT(x->vec.allocSize);											
	 bitVector * z = INIT(x->vec.allocSize);
	addBit(temp,i,1);
	align(x,temp,1);
	if(x->vec.size>0){														
		and_op(x,temp,z);													
		free(x->vec.data);													
		x->vec.data = z->vec.data;											
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												
	}
	else{																	
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
 void flip( bitVector *x, unsigned i){
	 bitVector * temp = INIT(x->vec.allocSize);											
	 bitVector * z = INIT(x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){														
		xor_op(x,temp,z);													
		free(x->vec.data);												
		x->vec.data = z->vec.data;											
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												
	}
	else{																	
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
 unsigned count( bitVector *v){
	unsigned count = 0;
	 run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){																												
		runDecode(&xrun);
		if(xrun.isFill && xrun.fill>0)
			count+=align_bit_size-1*xrun.nWords;
		else if(!xrun.isFill){
			align_t temp = (*xrun.it);
			for(int i = 0; i<align_bit_size; i++){
				if( temp & FILL_ZERO)
					count++;
				temp = temp<<1;
			}
			
		}
		xrun.it++;
	}
	return count;
}
 unsigned first( bitVector *v){
		unsigned index = 0;
		 run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){																												
			runDecode(&xrun);
			if(xrun.isFill && xrun.fill == 0)
				index +=align_bit_size-1*xrun.nWords;
			if(xrun.isFill && xrun.fill>0){
				return index;
			}
			else if(!xrun.isFill){
				align_t temp = (*xrun.it);
				for(int i = 0; i<align_bit_size; i++){ 
					if( temp & COUNTMASK){
						index += i;
						return index;
					}
					temp = temp>>1;
				}		
			}
			xrun.it++;
		}
		return index;
}
 unsigned last( bitVector *v){
		unsigned index = 0;
		unsigned lastIndex = 0;
		 run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){																												
			runDecode(&xrun);
			if(xrun.isFill){
				index +=(align_bit_size-1)*xrun.nWords;
				if(xrun.fill > 0)
					lastIndex = index;
			}
			else if(!xrun.isFill){
				align_t temp = (*xrun.it);
				for(int i = 0; i<(align_bit_size-1); i++){ 
					if( temp & COUNTMASK){
						lastIndex = index;
					}
					index++;
					temp = temp>>1;
				}		
			}
			xrun.it++;
		}
		return lastIndex;
}
 bitVector * setInt(unsigned i){
	bitVector *bv = create();
	set(bv,i);
	return bv;
}
 bitVector * Buf2RLE( unsigned char* rlerow, const unsigned clen){
	bitVector * v = create();
	v->vec.size= clen;
	memcpy(v->vec.data, rlerow, (size_t) clen );	
	return v;
}
 void RLE2Buf( unsigned char * set, unsigned char *rlerow, const unsigned clen )
{	
	 bitVector  *v = ( bitVector *) set;
	memcpy( rlerow, v->vec.data, (size_t) clen );		
	return;
}
 unsigned RLEsize( bitVector* v){
	return v->vec.size * WORDBYTES;
}
 void RLEfree( bitVector *v){
	FREE(v);
}
 void replace( bitVector* x, bitVector *y){

		free(x->vec.data);
		x->vec.data = y->vec.data;
		x->vec.size = y->vec.size;
		free(y);
}
 int test( bitVector *v, unsigned x){
	unsigned count = 0;
	 run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){																												
		runDecode(&xrun);
		if(xrun.isFill && xrun.fill>0){
			count+=(align_bit_size-1)*xrun.nWords;
			if(count > x)
				return 1;
		}
		else if(xrun.isFill && xrun.fill == 0){
			count+=(align_bit_size-1)*xrun.nWords;
			if(count > x)
				return 0;
		}
		else if(!xrun.isFill){
			align_t temp = (*xrun.it);
			for(int i = 0; i<(align_bit_size-1); i++){
				if(count == x)
				{
					if( temp & COUNTMASK)
						return 1;
					else
						return 0;
				}
				count++;
				temp = temp>>1;
			}
			
		}
		xrun.it++;
	}
	return 0;

}
 int eq( bitVector * x, bitVector *y){
	 run xrun,yrun; runInit(&xrun); runInit(&yrun);						
	xrun.it = x->vec.data;
	yrun.it = y->vec.data;
	while(xrun.it < x->vec.data + x->vec.size && yrun.it < y->vec.data+y->vec.size){																												
		runDecode(&xrun);
		runDecode(&yrun);
		if(xrun.isFill == yrun.isFill){
			if(xrun.fill != yrun.fill)
				return 0;
			if(xrun.isFill == 0 && yrun.isFill == 0)
				if(*(xrun.it) != *(yrun.it))
					return 0;
		}
		else
			return 0;
		xrun.it++;
		yrun.it++;
	}
	return 1;
}
int main(){return 0;}
