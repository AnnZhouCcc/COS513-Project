/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "timestamp-tag.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TimestampTag);

TypeId 
TimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ()
  ;
  return tid;
}
TypeId 
TimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
TimestampTag::GetSerializedSize (void) const
{
  return 8;
}
void 
TimestampTag::Serialize (TagBuffer buf) const
{
  buf.WriteU64 (time);
}
void 
TimestampTag::Deserialize (TagBuffer buf)
{
  time = buf.ReadU64 ();
}
void 
TimestampTag::Print (std::ostream &os) const
{
  os << "timestamp=" << time;
}
TimestampTag::TimestampTag ()
  : Tag () 
{
}

TimestampTag::TimestampTag (uint64_t timeNS)
  : Tag (),
    time (timeNS)
{
}

void
TimestampTag::SetTimestamp (uint64_t timeNS)
{
  time = timeNS;
}
uint64_t
TimestampTag::GetTimestamp (void)
{
  return time;
}

} // namespace ns3

