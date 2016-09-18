#ifndef __SI_REVERSE_INDEX_H__
#define __SI_REVERSE_INDEX_H__

#include "util/khash.h"
#include "key.h"

/* The reverse index is a helper to a usual index, that keeps track of the ids
 * and value tuples the index holds, and is used to transparently relocate index
 * records on updates */

static const int khSIId = 32;
KHASH_MAP_INIT_STR(khSIId, SIMultiKey *);
typedef khash_t(khSIId) SIReverseIndex;

SIReverseIndex *SI_NewReverseIndex();
void SIReverseIndex_Free(SIReverseIndex *i);

/* Return 1 if the id is already in the index and we should replace it */
int SIReverseIndex_Exists(SIReverseIndex *ri, SIId id, SIMultiKey **v);

/* Insert a record into the hash table. return 0 if there already existed a
 * record with the same id or 1 if not. The old record is discarded */
int SIReverseIndex_Insert(SIReverseIndex *ri, SIId id, SIMultiKey *k);

/* Delete a record from the index */
int SIReverseIndex_Delete(SIReverseIndex *ri, SIId id);

#endif
