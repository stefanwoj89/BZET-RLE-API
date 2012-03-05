#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
struct vector {
	unsigned size;
	unsigned allocSize;
	//unsigned data[];
	unsigned *data;
};
struct activeWord {
		unsigned value;
		unsigned nbits;
};
struct bitVector {
	struct activeWord active;
	struct vector vec;
};
#define SetBit(set,index)   set[index>>3] |= 0x80>>(index & 0x07)
#define PUSH( vect, value ) growIfNeeded(&vect), vect.data[vect.size++]=value
#define EMPTY( vect ) vect.size == 0
#define BACK( vect ) vect.data[vect.size-1]  
#define INIT( vector , vectorSize )\
	vector = (struct bitVector *) malloc(sizeof(bitVector)+ vectorSize * sizeof(unsigned)),\
	vector->vec.data = (unsigned *) malloc(vectorSize *sizeof(unsigned)),\
	memset(vector->vec.data, 0, vectorSize*sizeof(unsigned)),\
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
			unsigned newSize = sizeof(unsigned)* v->allocSize *2;			
			void * newMemory = realloc(v->data, newSize );
			assert(newMemory != NULL);
			v->data = (unsigned *) newMemory;
			v->allocSize *= 2;
		}																	
}

//Input: 31 literal bits stored in active.value.
//Output: vec extended by 31 bits.
void appendLiteral(struct bitVector* bv) { 															//bitVector::appendLiteral() {
	assert(bv->vec.size < bv->vec.allocSize);										
	if(EMPTY(bv->vec))																	//	IF (vec.empty())																								
		PUSH(bv->vec,bv->active.value);															//		vec.push back(active.value); 
	else if (bv->active.value == 0 )	{														//	ELSEIF (active.value == 0)
		if(BACK(bv->vec)==0)																//		IF (vec.back() == 0)
			BACK(bv->vec) = 0x80000002;														//			vec.back() = 0x80000002; 
		else if (BACK(bv->vec) >= 0x80000000 && BACK(bv->vec) < 0xC0000000)										//		ELSEIF (vec.back() >= 0x80000000 AND vec.back() < 0xC0000000)
			++BACK(bv->vec);															//			++vec.back(); // cbi = 4
		else																		//		ELSE
			PUSH(bv->vec,bv->active.value);														//			vec.push back(active.value) 
	}																							
	else if(bv->active.value == 0x7FFFFFFF ){														//	ELSEIF (active.value == 0x7FFFFFFF)																									
		if(BACK(bv->vec) == bv->active.value)														//		IF (vec.back() == active.value)																
				BACK(bv->vec) = 0xC0000002;													//			vec.back() = 0xC0000002; 																	
		else if(BACK(bv->vec) >= 0xC0000000)														//		ELSEIF (vec.back() >= 0xC0000000)
				++BACK(bv->vec);														//			++vec.back(); // cbi = 5				
		else																		//		ELSE
			PUSH(bv->vec,bv->active.value);														//			vec.push back(active.value); 
	}else																			//	ELSE
		PUSH(bv->vec,bv->active.value);															//		vec.push back(active.value); 
}																				//	}
//Input: n and fillBit, describing a fill with 31n bits of fillBit
//Output: vec extended by 31n bits of value fillBit.
void appendFill(struct bitVector* bv, unsigned n, unsigned fillBit){												//	bitVector::appendFill(n, fillBit) {
	assert( bv->active.nbits == 0);
	assert(n>0);																		//COMMENT: Assuming active.nbits = 0 and n > 0.
	if(n>1 && !EMPTY(bv->vec)){																//	IF (n > 1 AND ! vec.empty())
		if (fillBit == 0){																//		IF (fillBit == 0)
			if(BACK(bv->vec) >= 0x80000000 && BACK(bv->vec) < 0xC0000000)										//			IF (vec.back() >= 0x80000000 AND vec.back() < 0xC0000000)
				BACK(bv->vec) += n;														//				vec.back() += n; // cbi = 3
			else																	//			ELSE
				PUSH(bv->vec,0x80000000 + n);													//				vec.push back(0x80000000 + n); 
		}else if( BACK(bv->vec) >= 0xC0000000)														//		ELSEIF (vec.back()  0xC0000000)
			BACK(bv->vec) += n;															//			vec.back() += n; // cbi = 3
		else																		//		ELSE
			PUSH(bv->vec,0xC0000000 + n);														//			vec.push back(0xC0000000 + n); 
	}else if(EMPTY(bv->vec)){																//	ELSEIF (vec.empty())
		if(fillBit == 0)																//		IF (fillBit == 0)
			PUSH(bv->vec,0x80000000 + n);														//			vec.push back(0x80000000 + n); 
		else																		//		ELSE
			PUSH(bv->vec,0xC0000000 + n);														//			vec.push back(0xC0000000 + n); 
	}else{
																				//	ELSE
		bv->active.value = (fillBit?0x7FFFFFFF:0);													//active.value = (fillBit?0x7FFFFFFF:0); 
		appendLiteral(bv);																//		appendLiteral();
	}
}
																				//	}
struct run{																			//	class run { // used to hold basic information about a run
const unsigned * it;																		//		std::vector<unsigned>::const iterator it;
unsigned fill;																			//		unsigned fill; // one word-long version of the fill
unsigned nWords;																		//		unsigned nWords; // number of words in the run
unsigned isFill;																		//		bool isFill; // is it a fill run
};																				//	};


void runInit(struct run * r){r->it=0; r->fill=0; r->nWords=0; r->isFill=0; }											//	run::run() : it(0), fill(0), nWords(0), isFill(0) {};

void runDecode(struct run *r){																	//	run::decode() { // Decode the word pointed by iterator it
	if( *(r->it) > 0x7FFFFFFF){																//		IF (*it > 0x7FFFFFFF)
		r->fill = (*(r->it)>=0xC0000000?0x7FFFFFFF:0);													//			fill = (*it>=0xC0000000?0x7FFFFFFF:0), 
		r->nWords = *(r->it) & 0x3FFFFFFF;														//			nWords = *it & 0x3FFFFFFF,
		r->isFill = 1;																	//			isFill = 1;
	}else{																			//		ELSE
		r->nWords = 1;																	//			nWords = 1, 
		r->isFill = 0;																	//			isFill = 0;
	}
}//	}

inline unsigned andOper(unsigned x, unsigned y){return x&y;}
inline unsigned orOper(unsigned x, unsigned y){return x|y;}
inline unsigned xorOper(unsigned x, unsigned y){return x^y;}
void generic_op(struct bitVector * x, struct bitVector *y, struct bitVector *z, unsigned oper(unsigned, unsigned)){//	z = generic op(x, y) {
														//	Input: Two bitVector x and y containing the same number of bits.
														//	Output: The result of a bitwise logical operation as z.
	struct run xrun, yrun; runInit(&xrun); runInit(&yrun);							//		run xrun, yrun;
	xrun.it = x->vec.data; /*runDecode(&xrun);*/								//		xrun.it = x.vec.begin(); xrun.decode();
	yrun.it = y->vec.data; /*runDecode(&yrun);*/								//		yrun.it = y.vec.begin(); yrun.decode();
	while(xrun.it < x->vec.data + x->vec.size && yrun.it <y->vec.data + y->vec.size)	{																					//		WHILE (x.vec and y.vec are not exhausted) {
		if(xrun.nWords == 0) /*++xrun.it,*/ runDecode(&xrun);						//		IF (xrun.nWords == 0) ++xrun.it, xrun.decode();
		if(yrun.nWords == 0) /*++yrun.it,*/ runDecode(&yrun);						//		IF (yrun.nWords == 0) ++yrun.it, yrun.decode();
		if(xrun.isFill){										//		IF (xrun.isFill)
			if(yrun.isFill){									//			IF (yrun.isFill)
				unsigned nWords = MIN(xrun.nWords,yrun.nWords);					//				nWords = min(xrun.nWords, yrun.nWords),
				appendFill(z,nWords, oper(xrun.fill, yrun.fill)); //this is the correct way, not with iterators	//				z.appendFill(nWords, (*(xrun.it)  *(yrun.it))),
				//appendFill(z,nWords,oper(*(xrun.it),*(yrun.it)));				
				xrun.nWords -= nWords, yrun.nWords -= nWords;					//				xrun.nWords -= nWords, yrun.nWords -= nWords;
			}else{											//			ELSE
				z->active.value = oper(xrun.fill, *(yrun.it));					//				z.active.value = xrun.fill  *yrun.it,
				appendLiteral(z);								//				z.appendLiteral(),
				--yrun.nWords; // was missing from algorithm
				--xrun.nWords;									//				-- xrun.nWords;
			}
		}else if (yrun.isFill){										//		ELSEIF (yrun.isFill)
			z->active.value = oper(yrun.fill , *(xrun.it));						//			z.active.value = yrun.fill  *xrun.it,
			appendLiteral(z);									//			z.appendLiteral(),
			--yrun.nWords;
			--xrun.nWords; // was missing from algorithm
														//			-- yrun.nWords;
		}else{												//		ELSE
			z->active.value = oper(*(xrun.it) , *(yrun.it));					//			z.active.value = *xrun.it  *yrun.it,
			appendLiteral(z);//			z.appendLiteral();
			--yrun.nWords; // was missing from algorithm
			--xrun.nWords; // was missing from algorithm
		}
		if(xrun.nWords == 0) ++xrun.it;/*, runDecode(&xrun);*/						//		IF (xrun.nWords == 0) ++xrun.it, xrun.decode();
		if(yrun.nWords == 0) ++yrun.it;/*, runDecode(&yrun);*/	
	}													//	}
			z->active.value = oper(x->active.value, y->active.value);				//	z.active.value = x.active.value  y.active.value;
			z->active.nbits = x->active.nbits;							//	z.active.nbits = x.active.nbits;
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

unsigned nWords( struct bitVector * v){
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
void invert(struct bitVector * x){
	struct bitVector * mask,* z;
	INIT(mask,x->vec.allocSize); INIT(z,x->vec.allocSize); 
	unsigned xNumWords = nWords(x);

	appendFill(mask,xNumWords,0x7FFFFFFF);									// append a fill of all 1's for the same number of words.
	xor_op(x,mask,z);											// XOR operation with a mask of 1's is an inversion
	free(x->vec.data);											// free the old bitset
	x->vec.data = z->vec.data;										// address the inverted bitset
	free(z); FREE(mask);											// free the z data structure
}
void addBit(struct bitVector * v, unsigned i, unsigned inv){
	unsigned numFillOfZeros = i / 31;									// numFillofZeros is the number of FillWords before the literal word
	unsigned literalIndex = i % 31;										// Index of the set bit within the literal word
	unsigned literalValue = 1;
	unsigned fillBit = 0;
	literalValue = literalValue<<literalIndex;								// Set the bit at the Index position in the Literal Word
	if(inv){
		literalValue ^= 0x7FFFFFFF;
		fillBit =0x7FFFFFFF;
	}
	if(numFillOfZeros > 0) appendFill(v, numFillOfZeros,fillBit);						// Append Fillwords before the literal Word
	v->active.value = literalValue; v->active.nbits = 31;
	appendLiteral(v);											// Append the Literal Word
	v->active.value, v->active.nbits = 0;

}
void align(struct bitVector *x, struct bitVector *y,unsigned inv){
	int xNumWords = nWords(x);
	int yNumWords = nWords(y);										// A counter to determine number of words to append at the end of bitset
	int fillBit = 0;
	if(inv)
		fillBit= 0x7FFFFFFF;
	
	int appendSize = abs(xNumWords - yNumWords);
	if(xNumWords>yNumWords) appendFill(y,appendSize,fillBit);						// AppendFill words at the end
	else if(yNumWords>xNumWords) appendFill(x,appendSize,fillBit);
}
void set(struct bitVector * x, unsigned i)
{
	struct bitVector * temp, * z;										// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){											// if bitVector X was not empty
		or_op(x,temp,z);										// OR operation to get the new bitset with index i set
		free(x->vec.data);										// free the old bitset
		x->vec.data = z->vec.data;									// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);										// free the z data structure;
	}
	else{													// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
void unset(struct bitVector * x, unsigned i){
	struct bitVector * temp, * z;										// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp,i,1);
	align(x,temp,1);
	if(x->vec.size>0){											// if bitVector X was not empty
		and_op(x,temp,z);										// OR operation to get the new bitset with index i set
		free(x->vec.data);										// free the old bitset
		x->vec.data = z->vec.data;									// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);										// free the z data structure;
	}
	else{													// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
void flip(struct bitVector *x, unsigned i){
	struct bitVector * temp, * z;										// Initialization of temp (bitset with index i set) and z (the result of the or operation between x and temp)
	INIT(temp,x->vec.allocSize);INIT(z,x->vec.allocSize);
	addBit(temp, i, 0);
	align(x,temp,0);
	if(x->vec.size>0){											// if bitVector X was not empty
		xor_op(x,temp,z);										// OR operation to get the new bitset with index i set
		free(x->vec.data);										// free the old bitset
		x->vec.data = z->vec.data;									// address the inverted bitset
		x->vec.size = z->vec.size;
		free(z); FREE(temp);										// free the z data structure;
	}
	else{													// if bitVector X was empty
		free(x->vec.data);
		x->vec.data = temp->vec.data;
		x->vec.size = temp->vec.size;
		free(temp);
	}	
}
unsigned count(struct bitVector *v){
	unsigned count = 0;
	struct run xrun; runInit(&xrun);						
	xrun.it = v->vec.data; 																						
	while(xrun.it < v->vec.data + v->vec.size){								// Scan the bitset and find the total number of words.																					
		runDecode(&xrun);
		if(xrun.isFill && xrun.fill>0)
			count+=31*xrun.nWords;
		else if(!xrun.isFill){
			int temp = (*xrun.it);
			for(int i = 0; i<31; i++){
				if( temp & 0x80000000)
					count++;
				temp = temp<<1;
			}
			
		}
		xrun.it++;
	}
	return count;
}
unsigned first(struct bitVector *v){
		unsigned index = 0;
		struct run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){							// Scan the bitset and find the total number of words.																					
			runDecode(&xrun);
			if(xrun.isFill && xrun.fill == 0)
				index +=31*xrun.nWords;
			if(xrun.isFill && xrun.fill>0){
				index;
				return index;
			}
			else if(!xrun.isFill){
				int temp = (*xrun.it);
				for(int i = 0; i<31; i++){
					if( temp & 0x000000001){
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
unsigned last(struct bitVector *v){
		unsigned index = 0;
		unsigned lastIndex = 0;
		struct run xrun; runInit(&xrun);						
		xrun.it = v->vec.data; 																						
		while(xrun.it < v->vec.data + v->vec.size){							// Scan the bitset and find the total number of words.																					
			runDecode(&xrun);
			if(xrun.isFill){
				index +=31*xrun.nWords;
				if(xrun.fill > 0)
					lastIndex = index;
			}
			else if(!xrun.isFill){
				int temp = (*xrun.it);
				for(int i = 0; i<31; i++){
					if( temp & 0x000000001){
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

void test1(){
	struct bitVector * bv;
	INIT(bv, 10);
	PUSH(bv->vec,0x1000);
	PUSH(bv->vec,0x2000);
	PUSH(bv->vec,0x3000);
	printf("vec[0]=%#08X, vec[1]=%#08X, vec[2]=%#08X\n",bv->vec.data[0],bv->vec.data[1],bv->vec.data[2]);
	FREE(bv);
}
void test2(){
	struct bitVector * bv;
	INIT(bv, 10);
	PUSH(bv->vec,0x1000);
	PUSH(bv->vec,0x2000);
	PUSH(bv->vec,0x3000);
	printf("vec[0]=%#08X, vec[1]=%#08X, vec[2]=%#08X\n",bv->vec.data[0],bv->vec.data[1],bv->vec.data[2]);
	FREE(bv);
}
void andTest(){
	struct bitVector *A, *B,*C;
	INIT(A,10); INIT(B,10); INIT(C,10);

	PUSH(A->vec,0x40000380);
	PUSH(A->vec,0x80000002);
	PUSH(A->vec,0x001FFFFF);
	PUSH(A->vec,0x0000000F);
	PUSH(B->vec,0xC0000002);
	PUSH(B->vec,0x7C0001E0);
	PUSH(B->vec,0x3FE00000);
	PUSH(B->vec,0x00000003);
	PUSH(B->vec,0x00000003);
	PUSH(B->vec,0x00000003);
	PUSH(B->vec,0x00000003);
	and_op(A,B,C);
	printf("bitVector C after AND operation     is: %#010X %#010X %#010X\n", C->vec.data[0],C->vec.data[1],C->vec.data[2]);
	assert(C->vec.size == 3);
	assert(C->vec.data[0] == 0x40000380);
	assert(C->vec.data[1] == 0x80000003);
	assert(C->vec.data[2] == 0x00000003);
	FREE(A);FREE(B);FREE(C);
}
void invertTest(){
	struct bitVector *A;
	INIT(A,10);
	PUSH(A->vec,0x40000380);
	PUSH(A->vec,0x80000002);
	PUSH(A->vec,0x001FFFFF);
	PUSH(A->vec,0x0000000F);
	invert(A);
	FREE(A);
}
void setTest(){
	struct bitVector *A;
	INIT(A,10);
	set(A,100);
	printf("setTest1 result for set(100)        is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void setTest2(){
	struct bitVector *A;
	INIT(A,10);
	set(A,20);
	set(A,100);
	printf("setTest2 result for set([20,100])   is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void setTest3(){
	struct bitVector *A;
	INIT(A,10);
	set(A,100);
	set(A,20);
	printf("setTest3 result for set([100,20])   is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void setTest4(){
	struct bitVector *A;
	INIT(A,10);
	for(int i = 0; i<62; i++){
		set(A,i);
	}
	printf("setTest4 result for set([(0,61)])   is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void unsetTest(){
	struct bitVector *A;
	INIT(A,10);
	set(A,100);
	set(A,20);
	printf("unsetTest1 result for set([100,20]) is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	unset(A,100);
	printf("unsetTest1 result for unset(100)    is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void unsetTest2(){
	struct bitVector *A;
	INIT(A,10);
	set(A,100);
	set(A,20);
	printf("unsetTest2 result for set([100,20]) is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	unset(A,20);
	printf("unsetTest2 result for unset(20)     is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void flipTest(){
	struct bitVector *A;
	INIT(A,10);
	for(int i = 0; i<101; i++){
		set(A,i);
	}
	printf("flipTest result for set([0,100])    is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	flip(A,20);
	printf("flipTest result for flip(20)        is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	flip(A,50);
	printf("flipTest result for set(50)         is: %#010X %#010X %#010X\n",A->vec.data[0],A->vec.data[1],A->vec.data[2]);
	FREE(A);
}
void countTest(){
	struct bitVector *A;
	INIT(A,10);
	set(A,100);
	set(A,20);
	int countResult = count(A);
	printf("countTest result for set([100,20]) is: %d\n",countResult);
	FREE(A);
}
void firstTest(){
	struct bitVector *A;
	INIT(A,10);
	set(A,20);
	set(A,100);
	int firstResult = first(A);
	printf("firstTest result for set([20,100]) is: %d\n",firstResult);
	FREE(A);
}
void lastTest(){
	struct bitVector *A;
	INIT(A,10);
	for(int i = 20; i<101; i++){
		set(A,i);
	}
	int lastResult = last(A);
	printf("lastTest result for set([20,100]) is: %d\n",lastResult);
	FREE(A);
}
int main(int argc, char ** argv){
	//Test of use of the bitVector structure
	test1();
	test2();
	andTest();
	invertTest();
	setTest();
	setTest2();
	setTest3();
	setTest4();
	unsetTest();
	unsetTest2();
	flipTest();
	countTest();
	firstTest();
	lastTest();
}
