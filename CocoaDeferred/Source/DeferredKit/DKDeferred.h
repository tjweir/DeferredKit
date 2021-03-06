/*
 *  DKDeferred.h
 *  DeferredKit
 *
 *  Created by Samuel Sutch on 7/25/09.
 */

#import <Foundation/Foundation.h>
#import "DKCallback.h"
#import "DKMacros.h"


#define DKDeferredErrorDomain @"DKDeferred"
#define DKDeferredURLErrorDomain @"DKDeferredURLConnection"
#define DKDeferredCanceledError 419
#define DKDeferredGenericError 420
#define DKDeferredURLError 421
#define DKDeferredDeferredKey @"deferred"
#define DKDeferredResultKey @"result"
#define DKDeferredExceptionKey @"exception"

#define __CHAINED_DEFERRED_REUSE_ERROR [NSException \
	exceptionWithName:@"DeferredInstanceError" \
	reason:@"Chained deferreds can not be re-used" \
	userInfo:dict_(self, DKDeferredDeferredKey)]
#define __FINALIZED_DEFERRED_REUSE_ERROR [NSException \
	exceptionWithName:@"DeferredInstanceError" \
	reason:@"Finalized deferreds can not be re-used" \
	userInfo:dict_(self, DKDeferredDeferredKey)]
#define __CHAINED_DEFERRED_RESULT_ERROR [NSException \
	exceptionWithName:@"DeferredInstanceError" \
	reason:@"Deferred instances can only be chained " \
				 @"if they are the result of a callback" \
	userInfo:dict_(self, DKDeferredDeferredKey)]

/**
  * = DKDeferred =
  * 
  * A class to encapsulate a sequence of callbacks
  * in response to an object that may not yet be available.
  * In addition to responding to a callback a deferred also
  * keeps track of it's internal status:
  * 
  * -1 = not fired
  * 0  = success
  * 1  = error
  * 
  * It's design is greatly adopted from Twisted's Deferred class
  * and the library inspired by MochiKit's implementation of Deferred.
//  * Usage of this library requires DKDeferred be built with [FunctionalKit][0]
//  * and [json-framework][1].
  * 
  * == Primary Use ==
  * 
  * - (void)userTouchedGo:(id)sender {
  *			DKDeferred *d = [DKDeferred loadURL:@"http://google.com/"];
  *			[d addCallback:callbackTS(self, googleDidLoad:);
  *			[d addErrback:callbackTS(self, googleFailedToLoad:);
  * }
  *
  * - (id)googleDidLoad:(id)result { // in this case, an NSData object
  *			[loadingView removeFromSuperview];
  *			[webView loadHTMLString:[NSString stringWithUTF8String:[result bytes]]
  *											baseURL:[NSURL URLWithString:@"google.com"]];
  *			[view addSubview:webView];
  *			return nil;
  * }
  * 
  * - (id)googleFailedToLoad:(NSError *)result {
  *			// tell the user the internet is down.
	*			return nil;
  * }
  *     
  */
@interface DKDeferred : NSObject {
	NSMutableArray *chain;
	NSString *deferredID;
	int fired;
	int paused;
	NSMutableArray *results;
	id<DKCallback> canceller;
	BOOL silentlyCancelled;
	BOOL chained;
	BOOL finalized;
	id<DKCallback> finalizer;
}

@property(readonly) int fired;
@property(readonly) int paused;
@property(readonly) NSArray *results;
@property(readonly) BOOL silentlyCancelled;
@property(readwrite) BOOL chained;
@property(readonly) id<DKCallback> canceller;
@property(readonly) NSString *deferredID;
@property(readwrite, retain) id<DKCallback> finalizer;

// initializers
+ (DKDeferred *)deferred;
- (id)initWithCanceller:(id<DKCallback>)cancellerFunc;
// utility
+ (id)maybeDeferred:(id<DKCallback>)maybeDeferredf withObject:(id)anObject;
+ (id)gatherResults:(NSArray *)list_;
+ (id)succeed:(id)result;
+ (id)fail:(id)result;
+ (id)wait:(NSTimeInterval)seconds value:(id)value;
+ (id)callLater:(NSTimeInterval)seconds func:(id<DKCallback>)func;
+ (id)deferInThread:(id<DKCallback>)func withObject:(id)arg;
+ (id)loadURL:(NSString *)aUrl;
+ (id)loadURL:(NSString *)aUrl cached:(BOOL)cached;
// callback methods
- (id)addBoth:(id<DKCallback>)fn;
- (id)addCallback:(id<DKCallback>)fn;
- (id)addErrback:(id<DKCallback>)fn;
- (id)addCallbacks:(id<DKCallback>)cb :(id<DKCallback>)eb;
// control methods
- (void)cancel;
- (void)callback:(id)result;
- (void)errback:(id)result;

@end


/**
  * = DKDeferredList =
  * 
  * Wraps a series of deferreds into one deferred. Can be made
  * to callback on first result (the fireOnOneCallback/fireOnOneErrback) args
  */
@interface DKDeferredList : DKDeferred {
	NSArray *list;
	NSMutableArray *resultList;
	int finishedCount;
	BOOL fireOnOneCallback;
	BOOL fireOnOneErrback;
	BOOL consumeErrors;
}

@property(readwrite, assign) BOOL fireOnOneCallback;
@property(readwrite, assign) BOOL fireOnOneErrback;
@property(readwrite, assign) BOOL consumeErrors;
@property(readonly) int finishedCount;

// initializers
+ (id)deferredList:(NSArray *)list_;
+ (id)deferredList:(NSArray *)list_ withCanceller:(id<DKCallback>)cancelf;
- (id)initWithList:(NSArray *)list_
		 withCanceller:(id<DKCallback>)cancelf
 fireOnOneCallback:(BOOL)fireoc
	fireOnOneErrback:(BOOL)fireoe
		 consumeErrors:(BOOL)consume;
// internal callback used to callback/errback to contained deferreds
- (id)_cbDeferred:(id)index succeeded:(id)succeeded result:(id)result;

@end

/**
  * = DKThreadedDeferred = 
  * 
  * Wraps a threaded method call in a deferred interface.
  */
@interface DKThreadedDeferred : DKDeferred
{
	NSThread *thread;
	NSThread *parentThread;
	id<DKCallback> action;
}

@property(readonly) NSThread *thread;
@property(readonly) NSThread *parentThread;
@property(readonly) id<DKCallback> action;

// initializers
+ (DKThreadedDeferred *)threadedDeferred:(id<DKCallback>)func;
- (id)initWithFunction:(id<DKCallback>)func withObject:(id)arg;
- (id)initWithFunction:(id<DKCallback>)func withObject:(id)arg canceller:(id<DKCallback>)cancelf;
// internal methods used to run the function
- (void)_cbThreadedDeferred:(id)arg;
- (void)_cbReturnFromThread:(id)result;

@end

/**
 * = DKDeferredURLConnection =
 *
 * Wraps URL requests in a simplified deferred interface.
 */
@interface DKDeferredURLConnection : DKDeferred 
{
	NSString *url;
	NSMutableData *_data;
	NSURLConnection *connection;
	NSURLRequest *request;
	long expectedContentLength;
	double percentComplete;
	id<DKCallback> progressCallback;
	id<DKCallback> decodeFunction;
	NSTimeInterval refreshFrequency;
}

@property(nonatomic, readonly) NSString *url;
@property(nonatomic, readonly) NSData *data;
@property(nonatomic, readonly) long expectedContentLength;
@property(nonatomic, readonly) double percentComplete;
@property(nonatomic, readwrite, retain) id<DKCallback> progressCallback;
@property(nonatomic, readwrite, assign) NSTimeInterval refreshFrequency;

// initializers
+ (id)deferredURLConnection:(NSString *)aUrl;
- (id)initWithURL:(NSString *)aUrl;
- (id)initWithURL:(NSString *)aUrl pauseFor:(NSTimeInterval)pause;
- (id)initWithRequest:(NSURLRequest *)req 
						 pauseFor:(NSTimeInterval)pause
			 decodeFunction:(id<DKCallback>)decodeF;
// internal callbacks
- (id)_cbStartLoading:(id)result;
- (void)setProgressCallback:(id<DKCallback>)callback withFrequency:(NSTimeInterval)frequency;
- (void)_cbProgressUpdate;

@end


/**
  * = DKCache Protocol =
  * 
  * An as-of-now internally used caching protocol. Whatever backend used,
  * this serves as a permanantly adopted protocol. Anything can be cached if
  * it conforms to the NSCoding protocol.
  */
@protocol DKCache <NSObject>

@required
- (id)setValue:(NSObject *)_value forKey:(NSString *)_key 
			 timeout:(NSTimeInterval)_seconds; // deferred -> NSNumber
- (id)valueForKey:(NSString *)_key; // deferred -> NSObject
- (void)deleteValueForKey:(NSString *)_key; // deferred -> NSNumber
- (id)getManyValues:(NSArray *)_keys; // deferred -> NSDictionary
- (BOOL)hasKey:(NSString *)_key;
- (id)incr:(NSString *)_key delta:(int)delta; // nsnumber
- (id)decr:(NSString *)_key delta:(int)delta; // nsnumber

@end


/**
  * = DKDeferredCache =
  *
  * The current cache implementation used in DKDeferred. It implements
  * the DKCache protocol and uses a simple filesystem backend stored in
  * the users' applications cache directory.
  */
@interface DKDeferredCache : NSObject <DKCache>
{
	int maxEntries;
	int cullFrequency;
	NSString *dir;
	NSTimeInterval *defaultTimeout;
}

+ (id)sharedCache;
- (id)initWithDirectory:(NSString *)_dir 
						 maxEntries:(int)_maxEntries
					cullFrequency:(int)_cullFrequency;
- (id)_setValue:(NSObject *)value 
				 forKey:(NSString *)key
				timeout:(NSNumber *)timeout 
						arg:(id)arg;
- (id)_getValue:(NSString *)key;
- (id)_getManyValues:(NSArray *)keys;
- (void)_cull;
- (int)_getNumEntries;

@end

@interface NSObject(DKDeferredCache)

+ (BOOL)canBeStoredInCache;

@end
