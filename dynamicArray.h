/* 	dynArr.h : Dynamic Array implementation. */
#ifndef DYNAMIC_ARRAY_INCLUDED
#define DYNAMIC_ARRAY_INCLUDED 1


# ifndef TYPE
# define TYPE      int
# define TYPE_SIZE sizeof(int)
# endif

# ifndef LT
# define LT(A, B) ((A) < (B))
# endif

# ifndef EQ
# define EQ(A, B) ((A) == (B))
# endif



typedef struct DynArr DynArr;
struct DynArrIter;

struct bag;


/* Dynamic Array Functions */
DynArr *createDynArr(int cap);
void deleteDynArr(DynArr *v);

int sizeDynArr(DynArr *v);

void addDynArr(DynArr *v, TYPE val);
TYPE getDynArr(DynArr *v, int pos);
void putDynArr(DynArr *v, int pos, TYPE val);
void swapDynArr(DynArr *v, int i, int  j);
void removeAtDynArr(DynArr *v, int idx);
void addAtDynArr(DynArr *v, int idx, TYPE val);

/* Stack interface. */
int isEmptyDynArr(DynArr *v);
void pushDynArr(DynArr *v, TYPE val);
TYPE topDynArr(DynArr *v);
void popDynArr(DynArr *v);

/* Bag Interface */	
int containsDynArr(DynArr *v, TYPE val);
void removeDynArr(DynArr *v, TYPE val);

/* Ordered Bag Interface */

void addODynArr(DynArr *v, TYPE val);
int containsODynArr(DynArr *v, TYPE val);
void removeODynArr(DynArr *v, TYPE val);

/* Iterator Interface */

struct DynArrIter    *createDynArrIter(struct DynArr *v);
void initDynArrIter(struct DynArr *v, struct DynArrIter *itr);
int  hasNextDynArrIter(struct DynArrIter *itr);
TYPE    nextDynArrIter(struct DynArrIter *itr);
void	removeDynArrIter(struct DynArrIter *itr);

/*Bag Wrapper Interface*/
struct bag *createBag();
void addToBag(struct bag* b, TYPE val);
void removeFromBag(struct bag* b, TYPE val);
int containsBag(struct bag* b, TYPE val);
int isEmptyBag(struct bag* b);
void printBag(struct bag* b);

#endif
