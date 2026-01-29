//////////////////////////////////////////////////////////////////////
//
// BeeBEEP Copyright (C) 2010-2021 Marco Mastroddi
//
// BeeBEEP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// BeeBEEP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with BeeBEEP. If not, see <http://www.gnu.org/licenses/>.
//
// Author: Marco Mastroddi <marco.mastroddi(AT)gmail.com>
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "Chat.h"


Chat::Chat()
  : m_group(), m_messages(), m_lastMessageTimestamp(), m_unreadMessages( 0 ),
    m_unreadMessageUsersId(), m_unsavedMessages( false ), m_reactions()
{
}

Chat::Chat( const Chat& c )
{
  (void)operator=( c );
}

Chat& Chat::operator=( const Chat& c )
{
  if( this != &c )
  {
    m_group = c.m_group;
    m_messages = c.m_messages;
    m_lastMessageTimestamp = c.m_lastMessageTimestamp;
    m_unreadMessages = c.m_unreadMessages;
    m_unreadMessageUsersId = c.m_unreadMessageUsersId;
    m_unsavedMessages = c.m_unsavedMessages;
    m_reactions = c.m_reactions;
  }
  return *this;
}

bool Chat::isEmpty() const
{
  foreach( ChatMessage cm, m_messages )
  {
    if( cm.type() == ChatMessage::Chat || cm.type() == ChatMessage::FileTransfer || cm.type() == ChatMessage::History || cm.type() == ChatMessage::Voice )
      return false;
  }
  return true;
}

VNumber Chat::privateUserId() const
{
  foreach( VNumber user_id, m_group.usersId() )
  {
    if( user_id != ID_LOCAL_USER )
      return user_id;
  }
  return ID_INVALID;
}

void Chat::clearMessages()
{
  setLastMessageTimestamp( QDateTime() );
  readAllMessages();
  m_messages.clear();
}

void Chat::clearSystemMessages()
{
  QList<ChatMessage>::iterator it = m_messages.begin();
  bool skip_first_system_message = isDefault();
  while( it != m_messages.end() )
  {
    if( skip_first_system_message )
    {
      skip_first_system_message = false;
      ++it;
    }
    else if( it->isSystemActivity() )
      it = m_messages.erase( it );
    else
      ++it;
  }
}

bool Chat::hasSystemMessages() const
{
  int min_system_messages = isDefault() ? 2 : 1;
  int counter = 0;
  foreach( ChatMessage cm, m_messages )
  {
    if( cm.isSystemActivity() )
      counter++;

    if( counter > min_system_messages )
      return true;
  }
  return false;
}

int Chat::chatMessages() const
{
  int chat_messages = 0;
  foreach( ChatMessage cm, m_messages )
  {
    if( cm.type() == ChatMessage::Chat )
      chat_messages++;
  }
  return chat_messages;
}

void Chat::addMessage( const ChatMessage& cm )
{
  if( !cm.isValid() )
    return;

  m_messages.append( cm );
  if( cm.type() == ChatMessage::Chat )
  {
    m_unsavedMessages = true;

    if( cm.isFromLocalUser() )
    {
      foreach( VNumber user_id, m_group.usersId() )
      {
        if( user_id == ID_LOCAL_USER )
          continue;

        if( !m_unreadMessageUsersId.contains( user_id ) )
          m_unreadMessageUsersId.append( user_id );
      }
    }
  }
}

bool Chat::hasMinimumUsersForGroup() const
{
  int chat_members = 0;
  foreach( VNumber user_id, m_group.usersId() )
  {
    if( user_id != ID_LOCAL_USER )
      chat_members++;
  }
  return chat_members >= 2;
}

// Reaction support (Coal/Clawdbot enhancement)

QString Chat::messageKey( VNumber user_id, const QDateTime& timestamp )
{
  // Note: userId-based keys only work locally (not across machines)
  // For cross-machine compatibility, use the name-based overload
  return QString( "%1_%2" ).arg( user_id ).arg( timestamp.toString( Qt::ISODate ) );
}

QString Chat::messageKey( const QString& sender_name, const QDateTime& timestamp )
{
  // Name-based key is consistent across machines since both sides
  // see the same sender name and timestamp for a given message
  return QString( "%1_%2" ).arg( sender_name, timestamp.toUTC().toString( Qt::ISODate ) );
}

void Chat::addReaction( const QString& message_key, const QString& emoji, VNumber user_id )
{
  if( message_key.isEmpty() || emoji.isEmpty() )
    return;

  // Don't add duplicate reactions from the same user
  if( m_reactions.contains( message_key ) && m_reactions[message_key].contains( emoji ) )
  {
    if( m_reactions[message_key][emoji].contains( user_id ) )
      return;
  }

  m_reactions[message_key][emoji].append( user_id );
}

void Chat::removeReaction( const QString& message_key, const QString& emoji, VNumber user_id )
{
  if( !m_reactions.contains( message_key ) )
    return;
  if( !m_reactions[message_key].contains( emoji ) )
    return;

  m_reactions[message_key][emoji].removeAll( user_id );

  // Clean up empty entries
  if( m_reactions[message_key][emoji].isEmpty() )
    m_reactions[message_key].remove( emoji );
  if( m_reactions[message_key].isEmpty() )
    m_reactions.remove( message_key );
}

bool Chat::hasReactions( const QString& message_key ) const
{
  return m_reactions.contains( message_key ) && !m_reactions[message_key].isEmpty();
}

ReactionEmojiMap Chat::reactions( const QString& message_key ) const
{
  if( m_reactions.contains( message_key ) )
    return m_reactions[message_key];
  return ReactionEmojiMap();
}
