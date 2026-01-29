//////////////////////////////////////////////////////////////////////
//
// BeeBEEP Remote Control Server
// Allows external applications to send messages via Unix socket
//
// Added by: Coal (Clawdbot)
//
//////////////////////////////////////////////////////////////////////

#include "RemoteControl.h"
#include "Core.h"
#include "ChatManager.h"
#include "UserManager.h"
#include "Settings.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>

RemoteControl::RemoteControl( QObject* parent )
  : QObject( parent )
  , m_server( Q_NULLPTR )
{
}

RemoteControl::~RemoteControl()
{
  stop();
}

QString RemoteControl::socketPath()
{
  // Use the BeeBEEP data folder for the socket
  QString dataPath = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
  QDir dir( dataPath );
  if( !dir.exists() )
    dir.mkpath( "." );
  return dir.filePath( "control.sock" );
}

bool RemoteControl::start()
{
  if( m_server )
    return true;

  QString path = socketPath();
  
  // Remove stale socket file if it exists
  if( QFile::exists( path ) )
    QFile::remove( path );

  m_server = new QLocalServer( this );
  
  connect( m_server, &QLocalServer::newConnection, 
           this, &RemoteControl::onNewConnection );

  if( !m_server->listen( path ) )
  {
    emit logMessage( QString( "RemoteControl: Failed to start server at %1: %2" )
                     .arg( path, m_server->errorString() ) );
    delete m_server;
    m_server = Q_NULLPTR;
    return false;
  }

  emit logMessage( QString( "RemoteControl: Server listening at %1" ).arg( path ) );
  return true;
}

void RemoteControl::stop()
{
  if( m_server )
  {
    m_server->close();
    delete m_server;
    m_server = Q_NULLPTR;
  }

  foreach( QLocalSocket* client, m_clients )
  {
    client->disconnectFromServer();
    client->deleteLater();
  }
  m_clients.clear();

  // Clean up socket file
  QString path = socketPath();
  if( QFile::exists( path ) )
    QFile::remove( path );
}

bool RemoteControl::isRunning() const
{
  return m_server && m_server->isListening();
}

void RemoteControl::onNewConnection()
{
  while( m_server->hasPendingConnections() )
  {
    QLocalSocket* client = m_server->nextPendingConnection();
    m_clients.append( client );
    
    connect( client, &QLocalSocket::readyRead,
             this, &RemoteControl::onReadyRead );
    connect( client, &QLocalSocket::disconnected,
             this, &RemoteControl::onClientDisconnected );
    
    emit logMessage( "RemoteControl: Client connected" );
  }
}

void RemoteControl::onReadyRead()
{
  QLocalSocket* client = qobject_cast<QLocalSocket*>( sender() );
  if( !client )
    return;

  while( client->canReadLine() )
  {
    QByteArray line = client->readLine().trimmed();
    if( line.isEmpty() )
      continue;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson( line, &parseError );
    
    QJsonObject response;
    
    if( parseError.error != QJsonParseError::NoError )
    {
      response["ok"] = false;
      response["error"] = QString( "JSON parse error: %1" ).arg( parseError.errorString() );
    }
    else if( !doc.isObject() )
    {
      response["ok"] = false;
      response["error"] = "Expected JSON object";
    }
    else
    {
      response = processCommand( doc.object() );
    }

    QByteArray responseData = QJsonDocument( response ).toJson( QJsonDocument::Compact ) + "\n";
    client->write( responseData );
    client->flush();
  }
}

void RemoteControl::onClientDisconnected()
{
  QLocalSocket* client = qobject_cast<QLocalSocket*>( sender() );
  if( client )
  {
    m_clients.removeOne( client );
    client->deleteLater();
    emit logMessage( "RemoteControl: Client disconnected" );
  }
}

QJsonObject RemoteControl::processCommand( const QJsonObject& cmd )
{
  QString action = cmd["action"].toString().toLower();

  if( action == "send" )
    return handleSend( cmd );
  else if( action == "list_users" || action == "users" )
    return handleListUsers();
  else if( action == "status" )
    return handleStatus();
  else if( action == "list_chats" || action == "chats" )
    return handleListChats();
  else
  {
    QJsonObject response;
    response["ok"] = false;
    response["error"] = QString( "Unknown action: %1" ).arg( action );
    response["available_actions"] = QJsonArray::fromStringList( 
      QStringList() << "send" << "list_users" << "status" << "list_chats" );
    return response;
  }
}

QJsonObject RemoteControl::handleSend( const QJsonObject& cmd )
{
  QJsonObject response;
  
  QString to = cmd["to"].toString();
  QString message = cmd["message"].toString();
  
  if( to.isEmpty() )
  {
    response["ok"] = false;
    response["error"] = "Missing 'to' field (user name or nickname)";
    return response;
  }
  
  if( message.isEmpty() )
  {
    response["ok"] = false;
    response["error"] = "Missing 'message' field";
    return response;
  }

  // Check if connected
  if( !beeCore->isConnected() )
  {
    response["ok"] = false;
    response["error"] = "Not connected to network";
    return response;
  }

  // Find user by nickname
  User user = UserManager::instance().findUserByNickname( to );
  
  // If not found by nickname, try by path (Name@IP format)
  if( !user.isValid() )
    user = UserManager::instance().findUserByPath( to );
  
  // Try account name
  if( !user.isValid() )
    user = UserManager::instance().findUserByAccountName( to );

  if( !user.isValid() )
  {
    response["ok"] = false;
    response["error"] = QString( "User not found: %1" ).arg( to );
    
    // List available users as hint
    QJsonArray availableUsers;
    foreach( const User& u, UserManager::instance().userList().toList() )
    {
      if( !u.isLocal() )
      {
        QJsonObject userObj;
        userObj["nickname"] = u.name();
        userObj["path"] = u.path();
        userObj["connected"] = beeCore->isUserConnected( u.id() );
        availableUsers.append( userObj );
      }
    }
    response["available_users"] = availableUsers;
    return response;
  }

  // Check if user is connected
  if( !beeCore->isUserConnected( user.id() ) )
  {
    response["ok"] = false;
    response["error"] = QString( "User '%1' is not connected" ).arg( user.name() );
    return response;
  }

  // Get or create private chat with user
  Chat chat = ChatManager::instance().privateChatForUser( user.id() );
  
  if( !chat.isValid() )
  {
    // Create the private chat
    beeCore->createPrivateChat( user );
    chat = ChatManager::instance().privateChatForUser( user.id() );
  }

  if( !chat.isValid() )
  {
    response["ok"] = false;
    response["error"] = QString( "Failed to create chat with user: %1" ).arg( user.name() );
    return response;
  }

  // Send the message
  bool isImportant = cmd["important"].toBool( false );
  bool canBeDelayed = cmd["delayed"].toBool( true );
  bool isSourceCode = cmd["source_code"].toBool( false );
  
  int messagesSent = beeCore->sendChatMessage( chat.id(), message, isImportant, canBeDelayed, isSourceCode );

  response["ok"] = ( messagesSent > 0 );
  response["messages_sent"] = messagesSent;
  response["to_user"] = user.name();
  response["chat_id"] = QString::number( chat.id() );
  
  if( messagesSent == 0 )
    response["error"] = "Message was not sent (user may be offline)";

  return response;
}

QJsonObject RemoteControl::handleListUsers()
{
  QJsonObject response;
  QJsonArray users;
  
  foreach( const User& u, UserManager::instance().userList().toList() )
  {
    QJsonObject userObj;
    userObj["id"] = QString::number( u.id() );
    userObj["nickname"] = u.name();
    userObj["path"] = u.path();
    userObj["is_local"] = u.isLocal();
    userObj["connected"] = u.isLocal() ? true : beeCore->isUserConnected( u.id() );
    userObj["status"] = u.status();
    userObj["status_description"] = u.statusDescription();
    users.append( userObj );
  }
  
  response["ok"] = true;
  response["users"] = users;
  response["count"] = users.count();
  
  return response;
}

QJsonObject RemoteControl::handleStatus()
{
  QJsonObject response;
  
  response["ok"] = true;
  response["connected"] = beeCore->isConnected();
  response["connected_users"] = beeCore->connectedUsers();
  response["local_user"] = Settings::instance().localUser().name();
  response["socket_path"] = socketPath();
  
  return response;
}

QJsonObject RemoteControl::handleListChats()
{
  QJsonObject response;
  QJsonArray chats;
  
  foreach( const Chat& c, ChatManager::instance().constChatList() )
  {
    QJsonObject chatObj;
    chatObj["id"] = QString::number( c.id() );
    chatObj["name"] = c.name();
    chatObj["is_group"] = c.isGroup();
    chatObj["unread_messages"] = c.unreadMessages();
    
    QJsonArray members;
    foreach( VNumber userId, c.usersId() )
    {
      User u = UserManager::instance().findUser( userId );
      if( u.isValid() )
        members.append( u.name() );
    }
    chatObj["members"] = members;
    
    chats.append( chatObj );
  }
  
  response["ok"] = true;
  response["chats"] = chats;
  response["count"] = chats.count();
  
  return response;
}
