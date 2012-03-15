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
#define DLLEXPORT extern "C" __declspec(dllexport)
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

#define PUSH( vect, value ) growIfNeeded(&vect), vect.data[vect.size++]=value
#define EMPTY( vect ) vect.size == 0
#define BACK( vect ) vect.data[vect.size-1]  
#define INIT( vector , vectorSize )\
	vector = (struct bitVector *) malloc(sizeof(bitVector)),\
	vector->vec.data = (align_t *) malloc(vectorSize *sizeof(align_t)),\
	memset(vector->vec.data, 0, vectorSize*sizeof(align_t)),\
	vector->active.nbits = 0, \
	vector->active.value = 0,\
	vector->vec.size = 0 ,\
	vector->vec.allocSize = vectorSize;
#define FREE( vector ) \
	free(vector->vec.data),\
	free(vector);
#define MIN( value1, value2 ) ((value1)<(value2) ? (value1) : (value2))

inline int empty(struct vector *v){
	return (v->size == 0 ? 1 : 0);
}
void growIfNeeded(struct vector * v){
	if(v->size >= v->allocSize){
			unsigned newSize = sizeof(align_t)* v->allocSize*2;			
			void * newMemory = realloc(v->data, newSize );
			assert(newMemory != NULL);
			v->data = (align_t *) newMemory;
			v->allocSize *= 2;
		}																	
}

//Input: 31 literal bits stored in active.value.
//Output: vec extended by 31 bits.
void appendLiteral(struct bitVector* bv) { 															//bitVector::appendLiteral() {
	assert(bv->vec.size < bv->vec.allocSize);										
	if(EMPTY(bv->vec))																				//	IF (vec.empty())																								
		PUSH(bv->vec,bv->active.value);																//		vec.push back(active.value); 
	else if (bv->active.value == 0 )	{															//	ELSEIF (active.value == 0)
		if(BACK(bv->vec)==0)																		//		IF (vec.back() == 0)
			BACK(bv->vec) = (FILL_ZERO + 2);																//			vec.back() = 0x80000002; 
		else if (BACK(bv->vec) >= FILL_ZERO && BACK(bv->vec) < FILL_ZERO_MAX)							//		ELSEIF (vec.back() >= 0x80000000 AND vec.back() < 0xC0000000)
			++BACK(bv->vec);																		//			++vec.back(); // cbi = 4
		else																						//		ELSE
			PUSH(bv->vec,bv->active.value);															//			vec.push back(active.value) 
	}																							
	else if(bv->active.value == (align_t)FILL_ONES ){														//	ELSEIF (active.value == 0x7FFFFFFF)																									
		if(BACK(bv->vec) == bv->active.value)														//		IF (vec.back() == active.value)																
				BACK(bv->vec) = FILL_ONE + 2;															//			vec.back() = 0xC0000002; 																	
		else if(BACK(bv->vec) >= FILL_ONE && BACK(bv->vec) < FILL_ONE_MAX)														//		ELSEIF (vec.back() >= 0xC0000000)
				++BACK(bv->vec);																	//			++vec.back(); // cbi = 5				
		else																						//		ELSE
			PUSH(bv->vec,bv->active.value);															//			vec.push back(active.value); 
	}else																							//	ELSE
		PUSH(bv->vec,bv->active.value);																//		vec.push back(active.value); 
}																									//	}
//Input: n and fillBit, describing a fill with 31n bits of fillBit
//Output: vec extended by 31n bits of value fillBit.
inline void appendNewFills(struct bitVector* bv, unsigned long long n, align_t fillBase){
	while( n > 0)
	{
		unsigned long long will_append = MIN((align_t)WORD_COUNT_MASK, n);
		PUSH(bv->vec, fillBase + (align_t) will_append);
		n-=will_append;											
	}
}
void appendFill(struct bitVector* bv, unsigned long long n, align_t fillBit){								//	bitVector::appendFill(n, fillBit) {
	assert( bv->active.nbits == 0);
	assert(n>0);																					//COMMENT: Assuming active.nbits = 0 and n > 0.
	if(n>1 && !EMPTY(bv->vec)){																		//	IF (n > 1 AND ! vec.empty())
		if (fillBit == 0){																			//		IF (fillBit == 0)
			if(BACK(bv->vec) >= FILL_ZERO && BACK(bv->vec) < FILL_ZERO_MAX){							//			IF (vec.back() >= 0x80000000 AND vec.back() < 0xC0000000)
				align_t could_append = (align_t) WORD_COUNT_MASK - (BACK(bv->vec) & (align_t) WORD_COUNT_MASK);
				align_t will_append = align_t MIN(could_append, n);
				BACK(bv->vec) += will_append;
				n -= will_append;
			}
			appendNewFills(bv,n,FILL_ZERO);
		}else{ 
			if( BACK(bv->vec) >= FILL_ONE && BACK(bv->vec) < FILL_ONE_MAX){														//		ELSEIF (vec.back()  0xC0000000)
				align_t could_append = (align_t) WORD_COUNT_MASK - (BACK(bv->vec) & (align_t) WORD_COUNT_MASK);
				align_t will_append = align_t MIN(could_append, n);
				BACK(bv->vec) += will_append;
				n-= will_append;
			}							
			appendNewFills(bv,n,FILL_ONE);
		}
	}else if(EMPTY(bv->vec)){																		//	ELSEIF (vec.empty())
		if(fillBit == 0)																			//		IF (fillBit == 0)
			appendNewFills(bv,n,FILL_ZERO);															//			vec.push back(0x80000000 + n); 
		else																						//		ELSE
			appendNewFills(bv,n,FILL_ONE);														//			vec.push back(0xC0000000 + n); 
	}else{
																									//	ELSE
		bv->active.value = (fillBit?(align_t)FILL_ONES:0);													//active.value = (fillBit?0x7FFFFFFF:0); 
		growIfNeeded(&bv->vec);
		appendLiteral(bv);																			//		appendLiteral();
	}
}
																									//	}
struct run{																							//	class run { // used to hold basic information about a run
const align_t * it;																				//		std::vector<unsigned>::const iterator it;
align_t fill;																						//		unsigned fill; // one word-long version of the fill
align_t nWords;																					//		unsigned nWords; // number of words in the run
align_t isFill;																					//		bool isFill; // is it a fill run
};																									//	};


void runInit(struct run * r){r->it=0; r->fill=0; r->nWords=0; r->isFill=0; }						//	run::run() : it(0), fill(0), nWords(0), isFill(0) {};

void runDecode(struct run *r){																		//	run::decode() { // Decode the word pointed by iterator it
	if( *(r->it) > (align_t) FILL_ONES){																		//		IF (*it > 0x7FFFFFFF)
		r->fill = (*(r->it)>=FILL_ONE?(align_t)FILL_ONES:0);												//			fill = (*it>=0xC0000000?0x7FFFFFFF:0), 
		r->nWords = *(r->it) & (align_t) WORD_COUNT_MASK;															//			nWords = *it & 0x3FFFFFFF,
		r->isFill = 1;																				//			isFill = 1;
	}else{																							//		ELSE
		r->nWords = 1;																				//			nWords = 1, 
		r->isFill = 0;																				//			isFill = 0;
	}
}//	}

inline align_t andOper(align_t x, align_t y){return x&y;}
inline align_t orOper(align_t x, align_t y){return x|y;}
inline align_t xorOper(align_t x, align_t y){return x^y;}
void generic_op(struct bitVector * x, struct bitVector *y, struct bitVector *z, align_t oper(align_t, align_t)){				//	z = generic op(x, y) {
																																//	Input: Two bitVector x and y containing the same number of bits.
																																//	Output: The result of a bitwise logical operation as z.
	struct run xrun, yrun; runInit(&xrun); runInit(&yrun);																		//		run xrun, yrun;
	xrun.it = x->vec.data; /*runDecode(&xrun);*/																				//		xrun.it = x.vec.begin(); xrun.decode();
	yrun.it = y->vec.data; /*runDecode(&yrun);*/																				//		yrun.it = y.vec.begin(); yrun.decode();
	while(xrun.it < x->vec.data + x->vec.size && yrun.it <y->vec.data + y->vec.size)	{										//		WHILE (x.vec and y.vec are not exhausted) {
		if(xrun.nWords == 0) /*++xrun.it,*/ runDecode(&xrun);																	//		IF (xrun.nWords == 0) ++xrun.it, xrun.decode();
		if(yrun.nWords == 0) /*++yrun.it,*/ runDecode(&yrun);																	//		IF (yrun.nWords == 0) ++yrun.it, yrun.decode();
		if(xrun.isFill){																										//		IF (xrun.isFill)
			if(yrun.isFill){																									//			IF (yrun.isFill)
				align_t nWords = MIN(xrun.nWords,yrun.nWords);	//should this matter?																//				nWords = min(xrun.nWords, yrun.nWords),
				appendFill(z,nWords, oper(xrun.fill, yrun.fill));				//this is the correct way, not with iterators	//				z.appendFill(nWords, (*(xrun.it)  *(yrun.it))),
				//appendFill(z,nWords,oper(*(xrun.it),*(yrun.it)));				
				xrun.nWords -= nWords, yrun.nWords -= nWords;																	//				xrun.nWords -= nWords, yrun.nWords -= nWords;
			}else{																												//			ELSE
				z->active.value = oper(xrun.fill, *(yrun.it));																	//				z.active.value = xrun.fill  *yrun.it,
				growIfNeeded(&z->vec);
				appendLiteral(z);																								//				z.appendLiteral(),
				--yrun.nWords; // was missing from algorithm
				--xrun.nWords;																									//				-- xrun.nWords;
			}
		}else if (yrun.isFill){																									//		ELSEIF (yrun.isFill)
			z->active.value = oper(yrun.fill , *(xrun.it));																		//			z.active.value = yrun.fill  *xrun.it,
			growIfNeeded(&z->vec);
			appendLiteral(z);																									//			z.appendLiteral(),
			--yrun.nWords;
			--xrun.nWords; // was missing from algorithm
																																//			-- yrun.nWords;
		}else{																													//		ELSE
			z->active.value = oper(*(xrun.it) , *(yrun.it));																	//			z.active.value = *xrun.it  *yrun.it,
			growIfNeeded(&z->vec);
			appendLiteral(z);//			z.appendLiteral();
			--yrun.nWords; // was missing from algorithm
			--xrun.nWords; // was missing from algorithm
		}
		if(xrun.nWords == 0) ++xrun.it;/*, runDecode(&xrun);*/																	//		IF (xrun.nWords == 0) ++xrun.it, xrun.decode();
		if(yrun.nWords == 0) ++yrun.it;/*, runDecode(&yrun);*/	
	}													//	}
			z->active.value = oper(x->active.value, y->active.value);															//	z.active.value = x.active.value  y.active.value;
			z->active.nbits = x->active.nbits;																					//	z.active.nbits = x.active.nbits;
}
DLLEXPORT struct bitVector * create(){
	struct bitVector * bv;
	INIT(bv, 10);
	return bv;
}
inline void and_op(struct bitVector * x, struct bitVector *y, struct bitVector *z){
generic_op(x,y,z,andOper);
}
inline void or_op(struct bitVector * x, struct bitVector *y, struct bitVector *z){
generic_op(x,y,z,orOper);
}
inline void xor_op(struct bitVector * x, struct bitVector *y, struct bitVector *z){
generic_op(x,y,z,xorOper);
}
DLLEXPORT inline struct bitVector * and_op_wrapper(struct bitVector *x, struct bitVector *y){
	bitVector *z = create();
	and_op(x,y,z);
	return z;
}
DLLEXPORT inline struct bitVector * or_op_wrapper(struct bitVector *x, struct bitVector *y){
	bitVector *z = create();
	or_op(x,y,z);
	return z;
}
DLLEXPORT inline struct bitVector * xor_op_wrapper(struct bitVector *x, struct bitVector *y){
	bitVector *z = create();
	xor_op(x,y,z);
	return z;
}
DLLEXPORT unsigned nWords( struct bitVector * v){
	unsigned totalNumWords = 0;
	struct run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){								// Scan the bitset and find the total number of words.																					
		runDecode(&xrun);
		totalNumWords += xrun.nWords;
		xrun.it++;
	}
	return totalNumWords;
}
DLLEXPORT void invert(struct bitVector * x){
	struct bitVector * mask,* z;
	INIT(mask,x->vec.allocSize); INIT(z,x->vec.allocSize); 
	unsigned xNumWords = nWords(x);

	appendFill(mask,xNumWords,(align_t)FILL_ONES);							// append a fill of all 1's for the same number of words.
	xor_op(x,mask,z);														// XOR operation with a mask of 1's is an inversion
	free(x->vec.data);														// free the old bitset
	x->vec.data = z->vec.data;												// address the inverted bitset
	free(z); FREE(mask);													// free the z data structure
}
DLLEXPORT struct bitVector * RLEnot(struct bitVector *x){
	struct bitVector * mask,* z;
	INIT(mask,x->vec.allocSize); INIT(z,x->vec.allocSize); 
	unsigned xNumWords = nWords(x);

	appendFill(mask,xNumWords,(align_t)FILL_ONES);									// append a fill of all 1's for the same number of words.
	xor_op(x,mask,z);														// XOR operation with a mask of 1's is an inversion	
	return z;
}
void add_n_bits(struct bitVector *bv, unsigned count, unsigned value)
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
struct  bitVector * range (unsigned from, unsigned to, unsigned stride){
	struct bitVector *v;
	INIT(v,10);
	if(from>0)
		add_n_bits(v,from-1,0);
	if(stride > 0){
		unsigned i=from;
		while(true) //find out if equal or less than equal
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
}
DLLEXPORT struct bitVector * range(unsigned from, unsigned to){
	struct bitVector *v;
	INIT(v,10);
	if(from>0)
		add_n_bits(v,from-1,0);
	add_n_bits(v,to-from,1);
	appendLiteral(v);
	v->active.nbits = 0;
	v->active.value = 0;
	return v;
}
void addBit(struct bitVector * v, unsigned i, unsigned inv){
	unsigned numFillOfZeros = i / (align_bit_size-1);										// numFillofZeros is the number of FillWords before the literal word
	unsigned literalIndex = i % (align_bit_size-1);											// Index of the set bit within the literal word
	align_t literalValue = 1;
	align_t fillBit = 0;
	literalValue = literalValue<<literalIndex;								// Set the bit at the Index position in the Literal Word
	if(inv){
		literalValue ^= (align_t)FILL_ONES;
		fillBit = (align_t)FILL_ONES;
	}
	if(numFillOfZeros > 0) appendFill(v, numFillOfZeros,fillBit);			// Append Fillwords before the literal Word
	
	v->active.value = literalValue; v->active.nbits = align_bit_size-1;
	appendLiteral(v);														// Append the Literal Word
	v->active.value = 0; v->active.nbits = 0;

}
DLLEXPORT void align(struct bitVector *x, struct bitVector *y,unsigned inv){
	int xNumWords = nWords(x);
	int yNumWords = nWords(y);												// A counter to determine number of words to append at the end of bitset
	align_t fillBit = 0;
	if(inv)
		fillBit= (align_t)FILL_ONES;
	int appendSize = abs(xNumWords - yNumWords);
	if(xNumWords>yNumWords) appendFill(y,appendSize,fillBit);				// AppendFill words at the end
	else if(yNumWords>xNumWords) appendFill(x,appendSize,fillBit);
}
DLLEXPORT void set(struct bitVector * x, unsigned i)
{
	struct bitVector * temp, * z;											// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){														// if bitVector X was not empty
		or_op(x,temp,z);													// OR operation to get the new bitset with index i set
		free(x->vec.data);			 										// free the old bitset
		x->vec.data = z->vec.data;											// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												// free the z data structure;
	}
	else{																	// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
DLLEXPORT void unset(struct bitVector * x, unsigned i){
	struct bitVector * temp, * z;											// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp,i,1);
	align(x,temp,1);
	if(x->vec.size>0){														// if bitVector X was not empty
		and_op(x,temp,z);													// OR operation to get the new bitset with index i set
		free(x->vec.data);													// free the old bitset
		x->vec.data = z->vec.data;											// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												// free the z data structure;
	}
	else{																	// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
DLLEXPORT void flip(struct bitVector *x, unsigned i){
	struct bitVector * temp, * z;											// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){														// if bitVector X was not empty
		xor_op(x,temp,z);													// OR operation to get the new bitset with index i set
		free(x->vec.data);													// free the old bitset
		x->vec.data = z->vec.data;											// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);												// free the z data structure;
	}
	else{																	// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
DLLEXPORT unsigned count(struct bitVector *v){
	unsigned count = 0;
	struct run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){								// Scan the bitset and find the total number of words.																					
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
DLLEXPORT unsigned first(struct bitVector *v){
		unsigned index = 0;
		struct run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){							// Scan the bitset and find the total number of words.																					
			runDecode(&xrun);
			if(xrun.isFill && xrun.fill == 0)
				index +=align_bit_size-1*xrun.nWords;
			if(xrun.isFill && xrun.fill>0){
				index;
				return index;
			}
			else if(!xrun.isFill){
				align_t temp = (*xrun.it);
				for(int i = 0; i<align_bit_size; i++){ //should this be 31 or 32?
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
DLLEXPORT unsigned last(struct bitVector *v){
		unsigned index = 0;
		unsigned lastIndex = 0;
		struct run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){							// Scan the bitset and find the total number of words.																					
			runDecode(&xrun);
			if(xrun.isFill){
				index +=(align_bit_size-1)*xrun.nWords;
				if(xrun.fill > 0)
					lastIndex = index;
			}
			else if(!xrun.isFill){
				align_t temp = (*xrun.it);
				for(int i = 0; i<(align_bit_size-1); i++){ //should this be 31 or 32?
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
DLLEXPORT struct bitVector * setInt(unsigned i){
	bitVector *bv = create();
	set(bv,i);
	return bv;
}
DLLEXPORT struct bitVector * Buf2RLE( unsigned char* rlerow, const unsigned clen){
	bitVector * v = create();
	v->vec.size= clen;
	memcpy(v->vec.data, rlerow, (size_t) clen );	
	return v;
}
DLLEXPORT void RLE2Buf( unsigned char * set, unsigned char *rlerow, const unsigned clen )
{	// This routine writes into a Python byte array
	// the internal rle structure of the set.
	bitVector  *v = (struct bitVector *) set;
	memcpy( rlerow, v->vec.data, (size_t) clen );		
	return;
}
DLLEXPORT unsigned RLEsize(struct bitVector* v){
	return v->vec.size * WORDBYTES;
}
DLLEXPORT void RLEfree(struct bitVector *v){
	FREE(v);
}
DLLEXPORT void replace(struct bitVector* x,struct bitVector *y){

		free(x->vec.data);
		x->vec.data = y->vec.data;
		x->vec.size = y->vec.size;
		free(y);
}
DLLEXPORT int test(struct bitVector *v, unsigned x){
	unsigned count = 0;
	struct run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){								// Scan the bitset and find the total number of words.																					
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
DLLEXPORT int eq(struct bitVector * x, struct bitVector *y){
	struct run xrun,yrun; runInit(&xrun); runInit(&yrun);						
	xrun.it = x->vec.data;
	yrun.it = y->vec.data;
	while(xrun.it < x->vec.data + x->vec.size && yrun.it < y->vec.data+y->vec.size){								// Scan the bitset and find the total number of words.																					
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
