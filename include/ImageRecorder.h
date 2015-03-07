#pragma once

#include "ImageSourceOpenNI.h"

#include <OpenNI.h>
namespace iez {
class ImageRecorder {
public:
	ImageRecorder(iez::ImageSourceOpenNI *src);
	~ImageRecorder();
private:
	openni::Recorder *m_recorder;
};
}