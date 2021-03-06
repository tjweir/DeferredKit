//
//  DKDeferred+UIKit.m
//  DeferredKit
//
//  Created by Samuel Sutch on 8/31/09.
//

#import "DKDeferred+UIKit.h"
#import "UIImage+DKDeferred.h"


@implementation DKDeferred (UIKitAdditions)

+ (id)loadImage:(NSString *)aUrl cached:(BOOL)cached {
	DKDeferred *d;
	if (cached)
		d = [[[DKDeferredCache sharedCache] valueForKey:aUrl]
				 addBoth:curryTS((id)self, @selector(_cachedLoadURLCallback:results:), aUrl)];
	else
		d = [self loadURL:aUrl];
		[d addBoth:curryTS((id)self, @selector(_loadImageCallback:results:), aUrl)];
	return d;
}

+ (id)loadImage:(NSString *)aUrl sizeTo:(CGSize)size cached:(BOOL)cached {
	DKDeferred *d;
	if (cached)
		d = [[[DKDeferredCache sharedCache] valueForKey:aUrl]
				 addBoth:curryTS((id)self, @selector(_uncachedURLLoadCallback:results:), aUrl)];
	else
		d = [self loadURL:aUrl];
	[[d addBoth:curryTS((id)self, @selector(_loadImageCallback:results:), aUrl)]
	 addBoth:curryTS((id)self, @selector(_resizeImageCallbackSize:url:cache:results:), 
									 array_(nsnf(size.width), nsnf(size.height)), aUrl, nsnb(cached))];
	return d;
}

+ (id)_loadImageCallback:(NSString *)url results:(id)_results {
	if (isDeferred(_results)) {
		return [_results addBoth:curryTS((id)self, @selector(_loadImageCallback:results:), url)];
	} else if ([_results isKindOfClass:[NSData class]]) {
		//		UIImage *im = [UIImage imageWithData:_results];
		//		NSLog(@"loadImageCallback:%@", im);
		return [UIImage imageWithData:_results];
	}
	return nil;
}

+ (id)_resizeImageCallbackSize:(NSArray *)size url:(NSString *)url cache:(id)shouldCache results:(id)_results {
	if (isDeferred(_results))
		return [_results addBoth:curryTS((id)self, @selector(_resizeImageCallbackSize:url:cache:results:), size, url, shouldCache)];
	if ([_results isKindOfClass:[UIImage class]]) {
		UIImage *img = [(UIImage *)_results scaleImageToSize:
										CGSizeMake([[size objectAtIndex:0] floatValue],
															 [[size objectAtIndex:0] floatValue])];
		if (boolv(shouldCache)) {
			[[DKDeferredCache sharedCache] 
			 setValue:UIImagePNGRepresentation(img)
			 forKey:url timeout:7200.0];
		}
		return img;
	}
	return nil;
}

@end
