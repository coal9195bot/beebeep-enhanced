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
// Modified by: Coal (Clawdbot) - Added reply-to-message support
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#ifndef BEEBEEP_CHATMESSAGE_H
#define BEEBEEP_CHATMESSAGE_H

#include "Config.h"
class Message;


class ChatMessage
{
public:
  enum Type { Header, System, Chat, Connection, UserInfo, FileTransfer, History, Other, ImagePreview, Autoresponder, Voice, NumTypes };

  ChatMessage();
  ChatMessage( const ChatMessage& );
  ChatMessage( VNumber user_id, const Message&, ChatMessage::Type, bool can_be_saved );
  static ChatMessage createVoiceMessage( VNumber user_id, const QString& msg, ChatMessage::Type, bool can_be_saved );

  virtual ~ChatMessage() {}

  ChatMessage& operator=( const ChatMessage& );

  inline bool isValid() const;
  inline bool isFromSystem() const;
  inline bool isFromLocalUser() const;
  inline bool isFromAutoresponder() const;
  inline bool alertCanBeSent() const;
  inline bool isImportant() const;
  inline bool isHeader() const;
  inline bool isFileTransfer() const;
  inline bool isImagePreview() const;
  inline bool isVoice() const;

  bool isChatActivity() const;
  bool isSystemActivity() const;

  inline VNumber userId() const;
  inline const QString& message() const;
  inline const QDateTime& timestamp() const;
  inline const QColor& textColor() const;
  inline ChatMessage::Type type() const;

  inline bool canBeSaved() const;

  inline bool isSourceCode() const;
  
  // Reply-to-message support
  inline bool hasReply() const;
  inline const QString& replyToText() const;
  inline void setReplyToText( const QString& );
  inline const QString& replyToSender() const;
  inline void setReplyToSender( const QString& );

  // Reaction support (Coal/Clawdbot enhancement)
  inline bool isReaction() const;
  inline const QString& reactionEmoji() const;
  inline void setReactionEmoji( const QString& );
  inline const QString& reactionTargetKey() const;
  inline void setReactionTargetKey( const QString& );
  inline bool reactionIsRemoval() const;
  inline void setReactionIsRemoval( bool );

protected:
  ChatMessage( VNumber user_id, const QString& msg, ChatMessage::Type, bool can_be_saved );
  void createFromMessage( const Message& );

private:
  VNumber m_userId;
  QString m_message;
  QDateTime m_timestamp;
  QColor m_textColor;
  Type m_type;
  bool m_isImportant;
  bool m_canBeSaved;
  bool m_isSourceCode;
  
  // Reply data
  QString m_replyToText;
  QString m_replyToSender;

  // Reaction data
  QString m_reactionEmoji;
  QString m_reactionTargetKey;
  bool m_reactionIsRemoval;
};


// Inline Functions
inline bool ChatMessage::isValid() const { return m_userId != ID_INVALID; }
inline bool ChatMessage::isFromSystem() const { return m_userId == ID_SYSTEM_MESSAGE; }
inline bool ChatMessage::isFromLocalUser() const { return m_userId == ID_LOCAL_USER; }
inline bool ChatMessage::isFromAutoresponder() const { return m_type == ChatMessage::Autoresponder; }
inline bool ChatMessage::alertCanBeSent() const { return m_isImportant || (!isFromLocalUser() && !isFromSystem() && (m_type == ChatMessage::Chat || m_type == ChatMessage::FileTransfer ||
                                                                                                                     m_type == ChatMessage::ImagePreview || m_type == ChatMessage::Autoresponder ||
                                                                                                                     m_type == ChatMessage::Voice) ); }
inline bool ChatMessage::isImportant() const { return m_isImportant; }
inline bool ChatMessage::isHeader() const { return m_type == ChatMessage::Header; }
inline bool ChatMessage::isFileTransfer() const { return m_type == ChatMessage::FileTransfer; }
inline bool ChatMessage::isImagePreview() const { return m_type == ChatMessage::ImagePreview; }
inline bool ChatMessage::isVoice() const { return m_type == ChatMessage::Voice; }
inline VNumber ChatMessage::userId() const { return m_userId; }
inline const QString& ChatMessage::message() const { return m_message; }
inline const QDateTime& ChatMessage::timestamp() const { return m_timestamp; }
inline const QColor& ChatMessage::textColor() const { return m_textColor; }
inline ChatMessage::Type ChatMessage::type() const { return m_type; }
inline bool ChatMessage::canBeSaved() const { return m_canBeSaved; }
inline bool ChatMessage::isSourceCode() const { return m_isSourceCode; }

// Reply inline functions
inline bool ChatMessage::hasReply() const { return !m_replyToText.isEmpty(); }
inline const QString& ChatMessage::replyToText() const { return m_replyToText; }
inline void ChatMessage::setReplyToText( const QString& new_value ) { m_replyToText = new_value; }
inline const QString& ChatMessage::replyToSender() const { return m_replyToSender; }
inline void ChatMessage::setReplyToSender( const QString& new_value ) { m_replyToSender = new_value; }

// Reaction inline functions
inline bool ChatMessage::isReaction() const { return !m_reactionEmoji.isEmpty(); }
inline const QString& ChatMessage::reactionEmoji() const { return m_reactionEmoji; }
inline void ChatMessage::setReactionEmoji( const QString& new_value ) { m_reactionEmoji = new_value; }
inline const QString& ChatMessage::reactionTargetKey() const { return m_reactionTargetKey; }
inline void ChatMessage::setReactionTargetKey( const QString& new_value ) { m_reactionTargetKey = new_value; }
inline bool ChatMessage::reactionIsRemoval() const { return m_reactionIsRemoval; }
inline void ChatMessage::setReactionIsRemoval( bool new_value ) { m_reactionIsRemoval = new_value; }

#endif // BEEBEEP_CHATMESSAGE_H
