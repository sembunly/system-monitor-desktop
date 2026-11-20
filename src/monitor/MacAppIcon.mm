#include "MacAppIcon.h"

#include <QImage>
#include <QPixmap>
#import <Cocoa/Cocoa.h>

QIcon macAppIcon(const QString &bundlePath)
{
    @autoreleasepool {
        NSString *path = [NSString stringWithUTF8String:bundlePath.toUtf8().constData()];
        NSImage *image = [[NSWorkspace sharedWorkspace] iconForFile:path];
        if (!image) return {};
        NSRect rect = NSMakeRect(0, 0, 64, 64);
        CGImageRef cgImage = [image CGImageForProposedRect:&rect context:nil hints:nil];
        if (!cgImage) return {};
        NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        NSData *png = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        if (!png) return {};
        const QImage decoded = QImage::fromData(static_cast<const uchar *>(png.bytes), int(png.length), "PNG");
        return QIcon(QPixmap::fromImage(decoded).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
