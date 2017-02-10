#ifndef _BULLETINMESSAGE_H
#define _BULLETINMESSAGE_H

#define MESSAGE_LENGTH 201
struct BulletinMessage {
  int messageIndex;
  char message[MESSAGE_LENGTH];
}

#endif
