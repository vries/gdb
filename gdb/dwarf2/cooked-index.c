/* DIE indexing 

   Copyright (C) 2021 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "dwarf2/cooked-index.h"
#include "dwarf2/read.h"
#include "cp-support.h"
#include "ada-lang.h"
#include "split-names.h"

/* Hash function for cooked_index_entry.  */

static hashval_t
hash_entry (const void *e)
{
  const cooked_index_entry *entry = (const cooked_index_entry *) e;
  return dwarf5_djb_hash (entry->canonical);
}

/* Equality function for cooked_index_entry.  */

static int
eq_entry (const void *a, const void *b)
{
  const cooked_index_entry *ae = (const cooked_index_entry *) a;
  const gdb::string_view *sv = (const gdb::string_view *) b;
  return (strlen (ae->canonical) == sv->length ()
	  && strncasecmp (ae->canonical, sv->data (), sv->length ()) == 0);
}

/* See cooked-index.h.  */

cooked_index::cooked_index ()
  : m_hash (htab_create_alloc (10, hash_entry, eq_entry, nullptr,
			       xcalloc, xfree))
{
}

/* See cooked-index.h.  */

const cooked_index_entry *
cooked_index::add (sect_offset die_offset, enum dwarf_tag tag,
		   cooked_index_flag flags, const char *name,
		   const cooked_index_entry *parent_entry,
		   dwarf2_per_cu_data *per_cu)
{
  m_start = add (die_offset, tag, flags, name, parent_entry, per_cu,
		 m_start);

  /* An explicitly-tagged main program should always override the
     implicit "main" discovery.  */
  if ((flags & IS_MAIN) != 0)
    m_main = m_start;
  else if (per_cu->lang != language_ada
	   && m_main == nullptr
	   && strcmp (name, "main") == 0)
    m_main = m_start;

  return m_start;
}

/* Helper function to bridge the gap between a libiberty hash table
   callback and the callback used by the "quick" methods.  */
template<typename T>
inline void
htab_traverse_noresize (htab_t tab, T callback)
{
  auto trampoline = [] (void **slot, void *datum) -> int
  {
    T *cb = (T *) datum;
    return (*cb) ((cooked_index_entry *) *slot);
  };

  htab_traverse_noresize (tab, trampoline, &callback);
}

/* See cooked-index.h.  */

void
cooked_index::traverse
     (gdb::function_view<bool (const cooked_index_entry *)> callback)
{
  htab_traverse_noresize (m_hash.get (), callback);
}

/* See cooked-index.h.  */

gdb::unique_xmalloc_ptr<char>
cooked_index::handle_gnat_encoded_entry (cooked_index_entry *entry)
{
  std::string canonical = ada_decode (entry->name, false);
  if (canonical.empty ())
    return {};
  std::vector<gdb::string_view> names = split_name (canonical.c_str ());
  gdb::string_view tail = names.back ();
  names.pop_back ();

  const cooked_index_entry *parent = nullptr;
  for (const auto &name : names)
    {
      uint32_t hashval = dwarf5_djb_hash (name);
      void **slot = htab_find_slot_with_hash (m_hash.get (), &name,
					      hashval, INSERT);
      /* CUs are processed in order, so we only need to check the most
	 recent entry.  */
      cooked_index_entry *last = (cooked_index_entry *) *slot;
      if (last == nullptr
	  || last->tag != DW_TAG_namespace
	  || last->per_cu != entry->per_cu)
	{
	  gdb::unique_xmalloc_ptr<char> new_name
	    = make_unique_xstrndup (name.data (), name.length ());
	  last = add (entry->die_offset, DW_TAG_namespace,
		      0, new_name.get (), parent,
		      entry->per_cu, last);
	  last->canonical = last->name;
	  m_names.push_back (std::move (new_name));
	  *slot = last;
	}

      parent = last;
    }

  entry->parent_entry = parent;
  return make_unique_xstrndup (tail.data (), tail.length ());
}

/* See cooked-index.h.  */

void
cooked_index::finalize ()
{
  auto hash_name_ptr = [] (const void *p)
    {
      const cooked_index_entry *entry = (const cooked_index_entry *) p;
      return htab_hash_pointer (entry->name);
    };

  auto eq_name_ptr = [] (const void *a, const void *b) -> int
    {
      const cooked_index_entry *ea = (const cooked_index_entry *) a;
      const cooked_index_entry *eb = (const cooked_index_entry *) b;
      return ea->name == eb->name;
    };

  /* We can use pointer equality here because names come from
     .debug_str, which will normally be unique-ified by the linker.
     Also, duplicates are relatively harmless -- they just mean a bit
     of extra memory is used.  */
  htab_up seen_names (htab_create_alloc (10, hash_name_ptr, eq_name_ptr,
					 nullptr, xcalloc, xfree));


  cooked_index_entry *next = nullptr;
  for (cooked_index_entry *entry = m_start; entry != nullptr; entry = next)
    {
      /* The 'next' field is updated at the end of the loop, so
	 preserve it here for the next iteration.  */
	  next = entry->next;

	  gdb_assert (entry->canonical == nullptr);
	  if ((entry->per_cu->lang != language_cplus
	       && entry->per_cu->lang != language_ada)
	      || (entry->flags & IS_LINKAGE) != 0)
	    entry->canonical = entry->name;
	  else
	    {
	      if (entry->per_cu->lang == language_ada)
		{
		  /* FIXME use a bcache here?? */
		  gdb::unique_xmalloc_ptr<char> canon_name
		    = handle_gnat_encoded_entry (entry);
		  if (canon_name == nullptr)
		    entry->canonical = entry->name;
		  else
		{
		  entry->canonical = canon_name.get ();
		  m_names.push_back (std::move (canon_name));
		}
		}
	      else
		{
		  void **slot = htab_find_slot (seen_names.get (), entry,
						INSERT);
		  if (*slot == nullptr)
		    {
		      gdb::unique_xmalloc_ptr<char> canon_name
			= cp_canonicalize_string (entry->name);
		      if (canon_name == nullptr)
			entry->canonical = entry->name;
		      else
			{
			  entry->canonical = canon_name.get ();
			  m_names.push_back (std::move (canon_name));
			}
		    }
		  else
		    {
		      const cooked_index_entry *other
			= (const cooked_index_entry *) *slot;
		      entry->canonical = other->canonical;
		    }
		}
	    }

      gdb_assert (entry->canonical != nullptr);
      gdb::string_view lookup_name (entry->canonical);
      uint32_t hashval = dwarf5_djb_hash (lookup_name);
      void **slot = htab_find_slot_with_hash (m_hash.get (), &lookup_name,
					      hashval, INSERT);
      entry->next = (cooked_index_entry *) *slot;
      *slot = entry;
    }

  /* We don't need this any more, so make sure it is unusable.  */
  m_start = nullptr;
}
