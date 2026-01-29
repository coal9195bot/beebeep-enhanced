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

#ifndef BEEBEEP_CHAT_H
#define BEEBEEP_CHAT_H

#include "Group.h"
#include "ChatMessage.h"

// Reaction data: maps messageKey -> emoji -> list of userIds
// messageKey format: "userId_timestamp" (e.g., "42_2024-01-15T10:30:00")
typedef QMap<QString, QList<VNumber>> ReactionEmojiMap;  // emoji -> [userId1, userId2, ...]
typedef QMap<QString, ReactionEmojiMap> ChatReactionMap;  // messageKey -> emoji map

class Chat
{
public:
  Chat();
  Chat( const Chat& );
  Chat( const Group& );

  Chat& operator=( const Chat& );
  inline bool operator==( const Chat& ) const;
  inline bool isValid() const;
  inline void setGroup( const Group& );
  inline const Group& group() const;
  inline VNumber id() const;
  inline void setName( const QString& );
  inline const QString& name() const;
  inline const QList<VNumber>& usersId() const;
  inline bool hasUser( VNumber ) const;
  inline bool hasUsers( const QList<VNumber>& ) const;
  inline bool addUser( VNumber );
  inline bool removeUser( VNumber );
  inline bool isPrivateForUser( VNumber ) const;
  inline bool isPrivate() const;
  VNumber privateUserId() const;
  inline const QDateTime& lastMessageTimestamp() const;
  inline void setLastMessageTimestamp( const QDateTime& );
  inline int unreadMessages() const;
  int chatMessages() const;
  inline void addUnreadMessage();
  inline void readAllMessages();
  inline const QList<ChatMessage>& messages() const;
  void addMessage( const ChatMessage& );
  void clearMessages();
  void clearSystemMessages();
  inline const QString& privateId() const;
  inline const QList<VNumber>& unreadMessageUsersId() const;
  inline void setReadMessagesByUser( VNumber );
  inline bool userHasReadMessages( VNumber ) const;
  inline void setLastModifiedToNow();
  inline void setLastModified( const QDateTime& );
  inline const QDateTime& lastModified() const;

  bool isEmpty() const;
  bool isDefault() const;
  inline bool isGroup() const;
  bool hasMinimumUsersForGroup() const;
  bool hasSystemMessages() const;
  inline void setMessagesSaved();
  inline bool hasUnsavedMessages() const;

  // Reaction support (Coal/Clawdbot enhancement)
  void addReaction( const QString& message_key, const QString& emoji, VNumber user_id );
  void removeReaction( const QString& message_key, const QString& emoji, VNumber user_id );
  bool hasReactions( const QString& message_key ) const;
  ReactionEmojiMap reactions( const QString& message_key ) const;
  inline const ChatReactionMap& allReactions() const;
  static QString messageKey( VNumber user_id, const QDateTime& timestamp );
  static QString messageKey( const QString& sender_name, const QDateTime& timestamp );

private:
  Group m_group;
  QList<ChatMessage> m_messages;
  QDateTime m_lastMessageTimestamp;
  int m_unreadMessages;
  QList<VNumber> m_unreadMessageUsersId;
  bool m_unsavedMessages;
  ChatReactionMap m_reactions;

};


// Inline Functions
inline bool Chat::operator==( const Chat& c ) const { return m_group.id() == c.m_group.id(); }
inline bool Chat::isValid() const { return m_group.isValid(); }
inline bool Chat::isDefault() const { return m_group.chatType() == Group::DefaultChat; }
inline void Chat::setGroup( const Group& new_value ) { m_group = new_value; }
inline const Group& Chat::group() const { return m_group; }
inline VNumber Chat::id() const { return m_group.id(); }
inline void  Chat::setName( const QString& new_value ) { m_group.setName( new_value ); }
inline const QString& Chat::name() const { return m_group.name(); }
inline const QList<VNumber>& Chat::usersId() const { return m_group.usersId(); }
inline const QString& Chat::privateId() const { return m_group.privateId(); }
inline bool Chat::isGroup() const { return m_group.chatType() == Group::GroupChat; }
inline bool Chat::hasUser( VNumber user_id ) const { return isDefault() ? true : m_group.hasUser( user_id ); }
inline bool Chat::hasUsers( const QList<VNumber>& users_id ) const { return isDefault() ? true : m_group.hasUsers( users_id ); }
inline bool Chat::addUser( VNumber user_id ) { return m_group.addUser( user_id ); }
inline bool Chat::removeUser( VNumber user_id ) { return m_group.removeUser( user_id ); }
inline bool Chat::isPrivateForUser( VNumber user_id ) const { return isPrivate() && m_group.hasUser( user_id ); }
inline bool Chat::isPrivate() const { return m_group.chatType() == Group::PrivateChat;  }
inline const QDateTime& Chat::lastMessageTimestamp() const { return m_lastMessageTimestamp; }
inline void Chat::setLastMessageTimestamp( const QDateTime& new_value ) { m_lastMessageTimestamp = new_value; }
inline int Chat::unreadMessages() const { return m_unreadMessages; }
inline void Chat::addUnreadMessage() { m_unreadMessages++; }
inline void Chat::readAllMessages() { m_unreadMessages = 0; }
inline const QList<ChatMessage>& Chat::messages() const { return m_messages; }
inline const QList<VNumber>& Chat::unreadMessageUsersId() const { return m_unreadMessageUsersId; }
inline void Chat::setReadMessagesByUser( VNumber user_id ) { m_unreadMessageUsersId.removeOne( user_id ); }
inline bool Chat::userHasReadMessages( VNumber user_id ) const { return !m_unreadMessageUsersId.contains( user_id ); }
inline void Chat::setLastModified( const QDateTime& new_value ) { m_group.setLastModified( new_value ); }
inline void Chat::setLastModifiedToNow() { m_group.setLastModified( QDateTime::currentDateTimeUtc() ); }
inline const QDateTime& Chat::lastModified() const { return m_group.lastModified(); }
inline void Chat::setMessagesSaved() { m_unsavedMessages = false; }
inline bool Chat::hasUnsavedMessages() const { return m_unsavedMessages; }
inline const ChatReactionMap& Chat::allReactions() const { return m_reactions; }

#endif // BEEBEEP_CHAT_H
