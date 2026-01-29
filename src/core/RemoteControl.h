//////////////////////////////////////////////////////////////////////
//
// BeeBEEP Remote Control Server
// Allows external applications to send messages via Unix socket
//
// Added by: Coal (Clawdbot)
//
//////////////////////////////////////////////////////////////////////

#ifndef BEEBEEP_REMOTECONTROL_H
#define BEEBEEP_REMOTECONTROL_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class RemoteControl : public QObject
{
  Q_OBJECT

public:
  explicit RemoteControl( QObject* parent = Q_NULLPTR );
  ~RemoteControl();

  bool start();
  void stop();
  bool isRunning() const;

  static QString socketPath();

signals:
  void logMessage( const QString& );

private slots:
  void onNewConnection();
  void onReadyRead();
  void onClientDisconnected();

private:
  QJsonObject processCommand( const QJsonObject& cmd );
  QJsonObject handleSend( const QJsonObject& cmd );
  QJsonObject handleListUsers();
  QJsonObject handleStatus();
  QJsonObject handleListChats();

  QLocalServer* m_server;
  QList<QLocalSocket*> m_clients;
};

#endif // BEEBEEP_REMOTECONTROL_H
