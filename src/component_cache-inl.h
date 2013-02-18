/*
 * component_cache-inl.h
 *
 *  Created on: Aug 23, 2012
 *      Author: Marc Thurley
 */

#ifndef COMPONENT_CACHE_INL_H_
#define COMPONENT_CACHE_INL_H_

CacheEntryID ComponentCache::storeAsEntry(CachedComponent &ccomp, CacheEntryID super_comp_id){
    CacheEntryID id;

    if (statistics_.cache_full()) {
        deleteEntries();
    }

    ccomp.set_creation_time(my_time_++);

    assert(!statistics_.cache_full());

    if (free_entry_base_slots_.empty()) {
        if (entry_base_.capacity() == entry_base_.size()) {
            entry_base_.reserve(2 * entry_base_.size());
        }
        entry_base_.push_back(&ccomp);
        id = entry_base_.size() - 1;
    } else {
        id = free_entry_base_slots_.back();
        assert(id < entry_base_.size());
        assert(entry_base_[id] == nullptr);
        free_entry_base_slots_.pop_back();
        entry_base_[id] = &ccomp;
    }

    entry(id).set_father(super_comp_id);
    add_descendant(super_comp_id, id);

    assert(hasEntry(id));
    assert(hasEntry(super_comp_id));

    statistics_.incorporate_cache_store(ccomp);

  #ifdef DEBUG
      for (unsigned u = 2; u < entry_base_.size(); u++)
            if (entry_base_[u] != nullptr) {
              assert(entry_base_[u]->father() != id);
              assert(entry_base_[u]->first_descendant() != id);
              assert(entry_base_[u]->next_sibling() != id);
            }
  #endif
    return id;
}

void ComponentCache::cleanPollutionsInvolving(CacheEntryID id) {
  CacheEntryID father = entry(id).father();
  if (entry(father).first_descendant() == id) {
    entry(father).set_first_descendant(entry(id).next_sibling());
  } else {
    CacheEntryID act_sibl = entry(father).first_descendant();
    while (act_sibl) {
      CacheEntryID next_sibl = entry(act_sibl).next_sibling();
      if (next_sibl == id) {
        entry(act_sibl).set_next_sibling(entry(next_sibl).next_sibling());
        break;
      }
      act_sibl = next_sibl;
    }
  }
  CacheEntryID next_child = entry(id).first_descendant();
  entry(id).set_first_descendant(0);
  while (next_child) {
    CacheEntryID act_child = next_child;
    next_child = entry(act_child).next_sibling();
    cleanPollutionsInvolving(act_child);
  }
  removeFromHashTable(id);
  eraseEntry(id);
}

void ComponentCache::removeFromHashTable(CacheEntryID id) {
  CacheBucket *p_bucket = bucketOf(entry(id));
  if(p_bucket)
    for (auto it = p_bucket->begin(); it != p_bucket->end(); it++)
      if (*it == id) {
        *it = p_bucket->back();
        p_bucket->pop_back();
        break;
      }
}

void ComponentCache::removeFromDescendantsTree(CacheEntryID id) {
  assert(hasEntry(id));
  // we need a father for this all to work
  assert(entry(id).father());
  assert(hasEntry(entry(id).father()));
  // two steps
  // 1. remove id from the siblings list
  CacheEntryID father = entry(id).father();
  if (entry(father).first_descendant() == id) {
    entry(father).set_first_descendant(entry(id).next_sibling());
  } else {
    CacheEntryID act_sibl = entry(father).first_descendant();
    while (act_sibl) {
      CacheEntryID next_sibl = entry(act_sibl).next_sibling();
      if (next_sibl == id) {
        entry(act_sibl).set_next_sibling(entry(next_sibl).next_sibling());
        break;
      }
      act_sibl = next_sibl;
    }
  }

  // 2. add the children of this one as
  //    siblings to the current siblings
  CacheEntryID act_child = entry(id).first_descendant();
  while (act_child) {
    CacheEntryID next_child = entry(act_child).next_sibling();
    entry(act_child).set_father(father);
    entry(act_child).set_next_sibling(entry(father).first_descendant());
    entry(father).set_first_descendant(act_child);
    act_child = next_child;
  }
}

void ComponentCache::storeValueOf(CacheEntryID id, const mpz_class &model_count) {
  CacheBucket *p_bucket = bucketOf(entry(id));
  if(!p_bucket){
    unsigned ofs = entry(id).hashkey() % table_.size();
    p_bucket = table_[ofs] = new CacheBucket();
    num_occupied_buckets_++;
  }
  // when storing the new model count the size of the model count
  // and hence that of the component will change
  statistics_.cache_bytes_memory_usage_ -= entry(id).SizeInBytes();

  entry(id).set_model_count(model_count);
  entry(id).set_creation_time(my_time_);
  p_bucket->push_back(id);
  statistics_.cache_bytes_memory_usage_ += entry(id).SizeInBytes();
}





#endif /* COMPONENT_CACHE_INL_H_ */
