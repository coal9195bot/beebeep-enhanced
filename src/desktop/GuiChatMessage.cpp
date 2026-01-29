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
// Modified by: Coal (Clawdbot) - Added iOS-style chat bubble styling
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "BeeUtils.h"
#include "GuiChatMessage.h"
#include <QUrl>
#include <QUrlQuery>
#include "Chat.h"
#include "ChatMessage.h"
#include "EmoticonManager.h"
#include "Settings.h"
#include "Protocol.h"
#include "Settings.h"
#include "UserManager.h"


static QString textImportantPrefix()
{
  return QString( "<font color=red><b>!!</b></font> " );
}

static QString textImportantSuffix()
{
  return QString( "" );
}

// ========== Bubble Theme System (Coal/Clawdbot enhancement) ==========
//
// Theme 0: iMessage  - Blue sent, gray received
// Theme 1: WhatsApp  - Green sent, white/dark received
// Theme 2: Telegram  - Blue sent, white/dark received (lighter blue)
// Theme 3: Classic   - No bubbles, plain text (original BeeBEEP style)

struct BubbleThemeColors {
  const char* sentBg;
  const char* sentText;
  const char* sentMeta;
  const char* recvBg;       // light mode
  const char* recvBgDark;   // dark mode
  const char* recvText;     // light mode
  const char* recvTextDark; // dark mode
  const char* recvMeta;
  const char* nameLightColor;
  const char* nameDarkColor;
};

static const BubbleThemeColors s_themes[] = {
  // 0: iMessage
  { "#007AFF", "#ffffff", "#d0e8ff",
    "#e5e5ea", "#48484a", "#000000", "#ffffff", "#8e8e93",
    "#007AFF", "#64d2ff" },
  // 1: WhatsApp
  { "#005C4B", "#ffffff", "#7cc7b5",
    "#d9fdd3", "#202c33", "#111b21", "#e9edef", "#667781",
    "#00a884", "#00a884" },
  // 2: Telegram
  { "#3390ec", "#ffffff", "#aed5f8",
    "#eef2f5", "#2b2b2b", "#000000", "#f0f0f0", "#8e8e93",
    "#3390ec", "#6ab3f3" },
  // 3: Classic (no bubbles, minimal styling)
  { "#ddeeff", "#000000", "#888888",
    "#f5f5f5", "#333333", "#000000", "#dddddd", "#888888",
    "#0055aa", "#66aaff" },
};

static const int NUM_THEMES = 4;

static const BubbleThemeColors& currentTheme()
{
  int idx = Settings::instance().chatBubbleTheme();
  if( idx < 0 || idx >= NUM_THEMES ) idx = 0;
  return s_themes[idx];
}

static QString bubbleColorSent()
{
  return QString( currentTheme().sentBg );
}

static QString bubbleColorReceived()
{
  bool dark = Settings::instance().useDarkStyle();
  return QString( dark ? currentTheme().recvBgDark : currentTheme().recvBg );
}

static QString bubbleTextColorSent()
{
  return QString( currentTheme().sentText );
}

static QString bubbleTextColorReceived()
{
  bool dark = Settings::instance().useDarkStyle();
  return QString( dark ? currentTheme().recvTextDark : currentTheme().recvText );
}

static QString bubbleMetaColorSent()
{
  return QString( currentTheme().sentMeta );
}

static QString bubbleMetaColorReceived()
{
  return QString( currentTheme().recvMeta );
}

static QString userNameColor()
{
  bool dark = Settings::instance().useDarkStyle();
  return QString( dark ? currentTheme().nameDarkColor : currentTheme().nameLightColor );
}

static int bubblePadding()
{
  return Settings::instance().chatBubbleCompactPadding() ? 6 : 12;
}

QString GuiChatMessage::datetimestampToString( const ChatMessage& cm, bool show_timestamp, bool show_datestamp )
{
  QString date_time_stamp_format = "";

  if( show_datestamp || cm.timestamp().date() != QDate::currentDate() )
  {
    date_time_stamp_format += QT_TRANSLATE_NOOP( "Date", "yyyy-MM-dd" );
  }

  if( show_timestamp )
  {
    if( !date_time_stamp_format.isEmpty() )
      date_time_stamp_format += QString( " " );
    if( Settings::instance().useMessageTimestampWithAP() )
    {
      date_time_stamp_format += QT_TRANSLATE_NOOP( "Date", "h:mm ap" );
    }
    else
    {
      date_time_stamp_format += QT_TRANSLATE_NOOP( "Date", "hh:mm" );
    }
  }

  return date_time_stamp_format.isEmpty() ? date_time_stamp_format : (Settings::instance().useMessageTimestampWithAP() ? QLocale("en_US").toString( cm.timestamp(), date_time_stamp_format ) : cm.timestamp().toString( date_time_stamp_format ));
}

QString GuiChatMessage::formatMessage( const User& u, const ChatMessage& cm, VNumber last_user_id, bool show_timestamp, bool show_datestamp,
                                       bool skip_system_message, bool show_message_group_by_user, bool use_your_name, bool use_chat_compact,
                                       int read_status,
                                       const ReactionEmojiMap& reactions )
{
  Q_UNUSED( skip_system_message );
  Q_UNUSED( show_message_group_by_user );
  Q_UNUSED( use_chat_compact );
  
  bool is_sent = u.isLocal();
  bool is_classic = Settings::instance().chatBubbleTheme() == 3;
  
  QString bubble_bg = is_sent ? bubbleColorSent() : bubbleColorReceived();
  QString text_color = is_sent ? bubbleTextColorSent() : bubbleTextColorReceived();
  QString meta_color = is_sent ? bubbleMetaColorSent() : bubbleMetaColorReceived();
  QString alignment = is_sent ? "right" : "left";
  int padding = bubblePadding();
  
  // Padding for bubble positioning - wide separation
  // Sent messages: lots of left padding, flush right
  // Received messages: flush left, lots of right padding
  int pad_left = is_sent ? 250 : 0;
  int pad_right = is_sent ? 0 : 250;
  
  // Classic theme: no side padding, simple layout
  if( is_classic )
  {
    pad_left = 0;
    pad_right = 0;
  }
  
  QString text_formatted = cm.message();

  if( cm.isSourceCode() )
  {
    text_formatted.replace( QChar( '&' ), QLatin1String( "&amp;" ) );
    text_formatted.replace( QChar( '<' ), QLatin1String( "&lt;" ) );
    text_formatted.replace( QChar( '>' ), QLatin1String( "&gt;" ) );
    text_formatted.prepend( "<pre style=\"font-family: monospace; margin: 4px 0;\">" );
    text_formatted.append( "</pre>" );
  }

  text_formatted.replace( QString( "  " ), QLatin1String( "&nbsp;&nbsp;" ) );
  text_formatted.replace( QChar( '\t' ), QLatin1String( "&nbsp;&nbsp;&nbsp;&nbsp;" ) );
  text_formatted.replace( QChar( '\r' ), QLatin1String( "" ) );
  text_formatted.replace( QChar( '\n' ), QLatin1String( "<br>" ) );

  QString user_name_to_show = Bee::userNameToShow( u, true );
  QString date_time_stamp = datetimestampToString( cm, show_timestamp, show_datestamp );
  
  QString user_name = (u.isLocal() && !use_your_name) ? QObject::tr( "You" ) : (u.isValid() ? user_name_to_show : QObject::tr( "Unknown" ));
  if( cm.isFromAutoresponder() )
  {
    user_name = QString( "%1 from %2" ).arg( Settings::instance().autoresponderName(), user_name );
  }

  // Determine if we should show the username (not for consecutive messages from same user)
  bool show_user_name = (last_user_id != u.id()) || cm.isImportant() || cm.isFromAutoresponder();
  
  // Important message prefix/suffix
  QString important_prefix = cm.isImportant() ? textImportantPrefix() : QString();
  QString important_suffix = cm.isImportant() ? textImportantSuffix() : QString();

  // Build bubble using table structure
  QString html_message;
  
  if( is_classic )
  {
    // Classic theme: simple text, no bubble wrapping
    html_message = "";
    
    if( show_user_name )
    {
      html_message += QString( "<font color=\"%1\"><b>%2</b></font> " ).arg( userNameColor(), user_name );
    }
  }
  else
  {
    // Bubble themes: container table for alignment
    html_message = QString(
      "<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">"
      "<tr>"
      "<td width=\"%1\"></td>"
      "<td align=\"%2\">"
    ).arg( pad_left ).arg( alignment );
    
    // The bubble itself (Qt's QTextBrowser doesn't support border-radius)
    html_message += QString(
      "<table cellspacing=\"0\" cellpadding=\"%1\" border=\"0\" bgcolor=\"%2\">"
      "<tr><td>"
    ).arg( padding ).arg( bubble_bg );
    
    // Sender name for received messages (not for sent)
    if( show_user_name && !is_sent )
    {
      html_message += QString( "<font color=\"%1\" size=\"2\"><b>%2</b></font><br>" ).arg( userNameColor(), user_name );
    }
  }
  
  // Reply quote if present
  if( cm.hasReply() )
  {
    QString reply_bg = is_sent ? QString( "#0066cc" ) : (Settings::instance().useDarkStyle() ? QString( "#2c2c2e" ) : QString( "#d1d1d6" ));
    QString reply_text_col = is_sent ? QString( "#b8d4ff" ) : (Settings::instance().useDarkStyle() ? QString( "#a0a0a5" ) : QString( "#555555" ));
    
    QString reply_text = cm.replyToText();
    if( reply_text.length() > 80 )
      reply_text = reply_text.left( 80 ) + "...";
    
    if( is_classic )
    {
      html_message += QString(
        "<font color=\"%1\" size=\"2\"> | %2: <i>%3</i></font><br>"
      ).arg( reply_text_col, cm.replyToSender(), reply_text );
    }
    else
    {
      html_message += QString(
        "<table cellspacing=\"0\" cellpadding=\"4\" border=\"0\" bgcolor=\"%1\" width=\"100%%\" style=\"margin-bottom:4px;\">"
        "<tr><td><font color=\"%2\" size=\"2\">%3<br><i>%4</i></font></td></tr>"
        "</table>"
      ).arg( reply_bg, reply_text_col, cm.replyToSender(), reply_text );
    }
  }
  
  // Message content
  html_message += QString( "<font color=\"%1\">" ).arg( text_color );
  html_message += important_prefix;
  html_message += text_formatted;
  html_message += important_suffix;
  html_message += "</font>";
  
  // Timestamp, read receipts, and Reply link
  html_message += QString( "<br><font color=\"%1\" size=\"1\">" ).arg( meta_color );
  if( !date_time_stamp.isEmpty() )
  {
    html_message += date_time_stamp;
  }
  
  // Read receipt checkmarks for sent messages
  // read_status: 0=none, 1=sent (single check), 2=read (double check blue)
  if( is_sent && read_status > 0 )
  {
    if( !date_time_stamp.isEmpty() )
      html_message += " &nbsp;";
    
    if( read_status == 2 )
    {
      // Double blue checkmark - read
      html_message += QString( "</font><font color=\"#34B7F1\" size=\"1\">&#10003;&#10003;</font><font color=\"%1\" size=\"1\">" ).arg( meta_color );
    }
    else
    {
      // Single gray checkmark - sent but not read
      html_message += QString( "&#10003;" );
    }
  }
  
  if( !date_time_stamp.isEmpty() || (is_sent && read_status > 0) )
    html_message += " &nbsp;|&nbsp; ";
  
  // Add reply link (encode message for URL)
  QString encoded_text = QUrl::toPercentEncoding( text_formatted.left( 200 ) );
  QString encoded_sender = QUrl::toPercentEncoding( user_name );
  html_message += QString( "<a href=\"beebeep://reply?sender=%1&text=%2\" style=\"color:%3; text-decoration:none;\">↩ Reply</a>" )
    .arg( encoded_sender, encoded_text, meta_color );

  // React anchor - visible "React" link (Coal/Clawdbot enhancement)
  QString msg_key = Chat::messageKey( u.id(), cm.timestamp() );
  QString encoded_key = QUrl::toPercentEncoding( msg_key );
  html_message += QString( " &nbsp;|&nbsp; <a href=\"beebeep://react?key=%1\" style=\"color:%2; text-decoration:none;\">&#x2795;</a>" )
    .arg( encoded_key, meta_color );

  html_message += "</font>";

  // Reaction pills (Coal/Clawdbot enhancement)
  if( !reactions.isEmpty() )
  {
    html_message += reactionPillsHtml( reactions, is_sent );
  }
  
  if( is_classic )
  {
    // Classic: just add line break
    html_message += "<br>";
  }
  else
  {
    // Close bubble and container
    html_message += "</td></tr></table>";
    html_message += QString( "</td><td width=\"%1\"></td></tr></table>" ).arg( pad_right );
    
    // Add spacing between message groups
    if( last_user_id != u.id() || cm.isImportant() )
    {
      html_message.prepend( "<br>" );
    }
    
    html_message += "<br>";
  }

  return html_message;
}

QString GuiChatMessage::formatSystemMessage( const ChatMessage& cm, VNumber last_user_id, bool show_timestamp, bool show_datestamp, bool use_chat_compact )
{
  Q_UNUSED( use_chat_compact );
  
  if( cm.message().isEmpty() )
    return QString( "" );

  QString date_time_stamp = cm.type() != ChatMessage::ImagePreview ? datetimestampToString( cm, show_timestamp, show_datestamp ) : QString( "" );
  QString meta_color = Settings::instance().useDarkStyle() ? QString( "#8e8e93" ) : QString( "#8e8e93" );
  
  // System messages centered with subtle styling
  QString html_message = QString(
    "<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">"
    "<tr><td align=\"center\">"
    "<font color=\"%1\" size=\"2\">%2%3</font>"
    "</td></tr></table>"
  ).arg( meta_color,
         date_time_stamp.isEmpty() ? QString() : QString( "(%1) " ).arg( date_time_stamp ),
         cm.message() );

  if( cm.isImportant() )
  {
    html_message.prepend( textImportantPrefix() );
    html_message.append( textImportantSuffix() );
  }

  html_message.append( QLatin1String( "<br>" ) );

  if( cm.type() != ChatMessage::Other && last_user_id != ID_SYSTEM_MESSAGE )
  {
    html_message.prepend( QLatin1String( "<br>" ) );
  }
  else if( cm.isImportant() )
  {
    html_message.prepend( QLatin1String( "<br>" ) );
  }

  return html_message;
}

QString GuiChatMessage::chatToHtml( const Chat& c, bool skip_file_transfers, bool skip_system_message, bool force_timestamp, bool force_datestamp, bool use_chat_compact, bool skip_cannot_be_saved_messages )
{
  UserList chat_users;
  QString html_text = "";
  VNumber last_message_user_id = skip_system_message ? 0 : ID_SYSTEM_MESSAGE;

  if( c.isDefault() )
    chat_users = UserManager::instance().userList();
  else
    chat_users = UserManager::instance().userList().fromUsersId( c.usersId() );

  User u;

  foreach( ChatMessage cm, c.messages() )
  {
    if( cm.isFromSystem() )
    {
      if( cm.isFileTransfer() || cm.isImagePreview() )
      {
        if( skip_file_transfers )
          continue;
        if( skip_cannot_be_saved_messages && !cm.canBeSaved() )
          continue;
        html_text += formatSystemMessage( cm, last_message_user_id, force_timestamp || Settings::instance().chatShowMessageTimestamp(), force_datestamp, use_chat_compact );
      }
      else
      {
        if( skip_system_message )
          continue;
        if( skip_cannot_be_saved_messages && !cm.canBeSaved() )
          continue;
        html_text += formatSystemMessage( cm, last_message_user_id, force_timestamp || Settings::instance().chatShowMessageTimestamp(), force_datestamp, use_chat_compact );
      }
    }
    else
    {
      u = chat_users.find( cm.userId() );
      if( !u.isValid() )
      {
        u = UserManager::instance().findUser( cm.userId() );
        if( u.isValid() )
          chat_users.set( u );
      }
      
      // Determine read receipt status for sent messages
      int read_status = 0;
      if( u.isLocal() && cm.type() == ChatMessage::Chat && !c.isDefault() )
      {
        read_status = c.unreadMessageUsersId().isEmpty() ? 2 : 1;
      }
      
      // Look up reactions for this message (Coal/Clawdbot enhancement)
      QString msg_key = Chat::messageKey( cm.userId(), cm.timestamp() );
      ReactionEmojiMap msg_reactions = c.reactions( msg_key );

      html_text += formatMessage( u, cm, last_message_user_id, force_timestamp || Settings::instance().chatShowMessageTimestamp(), force_datestamp, skip_system_message, false, true, use_chat_compact, read_status, msg_reactions );
    }
    last_message_user_id = cm.isImportant() ? ID_IMPORTANT_MESSAGE : cm.userId();
  }

  return html_text;
}

// Reaction pills HTML rendering (Coal/Clawdbot enhancement)
QString GuiChatMessage::reactionPillsHtml( const ReactionEmojiMap& reactions, bool is_sent )
{
  if( reactions.isEmpty() )
    return QString();

  bool dark = Settings::instance().useDarkStyle();
  bool is_classic = Settings::instance().chatBubbleTheme() == 3;

  // Pill styling
  QString pill_bg = dark ? "#3a3a3c" : "#e8e8ed";
  QString pill_text_color = dark ? "#ffffff" : "#000000";
  QString pill_count_color = dark ? "#a0a0a5" : "#666666";

  QString html;

  if( is_classic )
  {
    // Classic: simple inline pills
    html += "<br>";
  }

  // Build pills: each emoji gets a small pill showing emoji + count
  html += QString( "<table cellspacing=\"2\" cellpadding=\"0\" border=\"0\"><tr>" );

  QMap<QString, QList<VNumber>>::const_iterator it;
  for( it = reactions.constBegin(); it != reactions.constEnd(); ++it )
  {
    QString emoji = it.key();
    int count = it.value().size();
    if( count <= 0 )
      continue;

    QString count_text = count > 1 ? QString( "&nbsp;<font color=\"%1\" size=\"1\">%2</font>" ).arg( pill_count_color ).arg( count ) : QString();

    html += QString(
      "<td bgcolor=\"%1\">"
      "&nbsp;%2%3&nbsp;"
      "</td><td width=\"2\"></td>"
    ).arg( pill_bg, emoji, count_text );
  }

  html += "</tr></table>";

  return html;
}
