//
//  PPlayer_C_Interface
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/15.
//  Copyright © 2019年 邱开禄. All rights reserved.
//

#ifndef PPlayer_C_Interface_H
#define PPlayer_C_Interface_H
#include "Message.h"
NS_MEDIA_BEGIN
typedef void (*msg_loop) (void* playerInstance, Message &msg);
NS_MEDIA_END
#endif
