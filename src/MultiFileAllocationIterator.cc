/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "MultiFileAllocationIterator.h"
#include "MultiDiskAdaptor.h"
#include "FileEntry.h"
#include "SingleFileAllocationIterator.h"
#include "DiskWriter.h"

namespace aria2 {

MultiFileAllocationIterator::MultiFileAllocationIterator(MultiDiskAdaptor* diskAdaptor):
  _diskAdaptor(diskAdaptor),
  _entries(_diskAdaptor->diskWriterEntries),
  _offset(0)
{}

MultiFileAllocationIterator::~MultiFileAllocationIterator() {}

void MultiFileAllocationIterator::allocateChunk()
{
  while(_fileAllocationIterator.isNull() || _fileAllocationIterator->finished()) {
    if(_entries.empty()) {
      break;
    }
    DiskWriterEntryHandle entry = _entries.front();
    _entries.pop_front();
    FileEntryHandle fileEntry = entry->getFileEntry();
    // Open file before calling DiskWriterEntry::size()
    _diskAdaptor->openIfNot(entry, &DiskWriterEntry::openFile);
    entry->enableDirectIO();
    if(entry->needsFileAllocation() && entry->size() < fileEntry->getLength()) {
      // Calling private function of MultiDiskAdaptor.
      _fileAllocationIterator.reset
	(new SingleFileAllocationIterator(entry->getDiskWriter().get(),
					  entry->size(),
					  fileEntry->getLength()));
      _fileAllocationIterator->init();
    }
  }
  if(finished()) {
    return;
  }
  _fileAllocationIterator->allocateChunk();
}

bool MultiFileAllocationIterator::finished()
{
  return _entries.empty() && (_fileAllocationIterator.isNull() || _fileAllocationIterator->finished());
}

off_t MultiFileAllocationIterator::getCurrentLength()
{
  if(_fileAllocationIterator.isNull()) {
    return 0;
  } else {
    return _fileAllocationIterator->getCurrentLength();
  }
}

uint64_t MultiFileAllocationIterator::getTotalLength()
{
  if(_fileAllocationIterator.isNull()) {
    return 0;
  } else {
    return _fileAllocationIterator->getTotalLength();
  }
}

const DiskWriterEntries& MultiFileAllocationIterator::getDiskWriterEntries() const
{
  return _entries;
}

} // namespace aria2
