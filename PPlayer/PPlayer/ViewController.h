//
//  ViewController.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

#define SCREENWIDTH         ([UIScreen mainScreen].bounds.size.width)
#define SCREENHEIGHT        ([UIScreen mainScreen].bounds.size.height)
#define SCREENWIDTH_D40     (SCREENWIDTH / 40)
#define SCREENHEIGHT_D40    (SCREENHEIGHT / 40)

@property (nonatomic, strong) UIButton*         pOpenVideoButton;

@end

