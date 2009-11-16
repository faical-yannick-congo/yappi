/*
*    An Adaptive Hash Table
*    Sumer Cip 2009
*/

#include "_yhashtab.h"
#include "_ymem.h"

static int
_hgrow(_htab *ht)
{
    int i;       
    _htab *dummy; 
    _hitem *p, *next;   

    dummy = htcreate(ht->logsize+1, ht->growfactor);
    if (!dummy)
		return 0;
    for(i=0;i<ht->realsize;i++) {
        p = ht->_table[i];       
        while(p) {
            next = p->next;
            if (!hadd(dummy, p->key, p->val))
            	return 0;
            yfree(p);           
            p = next;
        }
    }       
   
    yfree(ht->_table);   
    ht->_table = dummy->_table;
    ht->logsize = dummy->logsize;
    ht->realsize = dummy->realsize;
    ht->mask = dummy->mask;
    yfree(dummy);
    return 1;
}

unsigned int
_hhash(_htab *ht, int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return (unsigned int)(a & ht->mask);
}


_htab *
htcreate(int logsize, int growfactor)
{
    int i;
    _htab *ht;
   
    ht = (_htab *)ymalloc(sizeof(_htab));
    if (!ht)
		return NULL;
    ht->logsize = logsize;
    ht->realsize = HSIZE(logsize);
    ht->mask = HMASK(logsize);
    ht->growfactor = growfactor;
    ht->count = 0;
    ht->freecount = 0;
    ht->_table = (_hitem **)ymalloc(ht->realsize * sizeof(_hitem *));
    if (!ht->_table) {
    	yfree(ht);
    	return NULL;
    }

    for(i=0;i<ht->realsize;i++)
        ht->_table[i] = NULL;    
           
    return ht;
}


void
htdestroy(_htab *ht)
{
    int i;
    _hitem *p, *next;
   
    for(i=0;i<ht->realsize;i++) {
        p = ht->_table[i];
        while(p) {
            next = p->next;           
            yfree(p);
            p = next;
        }
    }
    
    yfree(ht->_table);
    yfree(ht);
}


int
hadd(_htab *ht, int key, int val)
{
    int h;
    _hitem *new,*p;
   
    h = _hhash(ht, key);
    p = ht->_table[h];
    new = NULL;
    while(p) {
        if (p->key == key)
            return 0;    
        if (p->free)
            new = p;   
        p = p->next;   
    }       
    // have a free slot?
    if (new) {
        new->key = key;
        new->val = val;
        new->free = 0;
        ht->freecount--;
    } else {       
        new = (_hitem *)ymalloc(sizeof(_hitem));
        if (!new)
        	return 0;
        new->key = key;
        new->val = val;
        new->next = ht->_table[h]; // add to front
        new->free = 0;
        ht->_table[h] = new;
        ht->count++;
    }
    // need resizing?   
    if ((ht->count - ht->freecount) > (ht->realsize*ht->growfactor)) {
        if (!_hgrow(ht))
			return 0;
    }
    return 1;
}


_hitem *
hfind(_htab *ht, int key)
{
    int h;    
    _hitem *p, *prev;
       
    h = _hhash(ht, key);
    p = ht->_table[h];
    if (!p)
        return NULL;
    if ((p->key == key) && (!p->free))   
        return p;
   
    prev = p;
    p=p->next;
    while(p) {
        if ((p->key == key) && (!p->free)) {
            SWAP(prev->key, p->key);
            SWAP(prev->val, p->val);   
            SWAP(prev->free, p->free);   
            return prev;
        }   
        prev = p;
        p = p->next;
    }
    return NULL;
}

// enums non-free items
void
henum(_htab *ht, int (*enumfn)(_hitem *item, void *arg), void *arg)
{
    int rc, i;
    _hitem *p, *next;
   
    for(i=0;i<ht->realsize;i++) {       
        p = ht->_table[i];
        while(p) {
            next = p->next;
			if (!p->free) {
				rc = enumfn(p, arg); // item may be freed.
				if(rc)
					return;
			}
            p = next;       
        }
    }       
}

int
hcount(_htab *ht)
{
    return (ht->count - ht->freecount);
}

void
hfree(_htab *ht, _hitem *item)
{
    item->free = 1;
    ht->freecount++;   
}