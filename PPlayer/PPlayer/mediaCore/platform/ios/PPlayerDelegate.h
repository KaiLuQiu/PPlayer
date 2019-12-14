//
//  PPlayerDelegate.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/11.
//  Copyright © 2019年 邱开禄. All rights reserved.
//

#import <UIKit/UIKit.h>
#ifndef PPlayerDelegate_H
#define PPlayerDelegate_H

@protocol OnPreparedListener <NSObject>
-(void) onPrepared;
@end

@protocol OnCompletionListener <NSObject>
-(void) onCompletion;
@end

@protocol OnSeekCompletionListener <NSObject>
-(void) onCompletion;
@end

@protocol OnErrorListener <NSObject>
-(void) onCompletion;
@end

@protocol OnInfoListener <NSObject>
-(void) onCompletion;
@end

#endif
