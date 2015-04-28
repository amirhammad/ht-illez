#pragma once

#include "ImageSourceOpenNI.h"

#include <OpenNI.h>

namespace iez {
class ImageRecorder {
public:
	ImageRecorder();

	void init(iez::ImageSourceOpenNI *src, QString path);

	~ImageRecorder();
private:
	openni::Recorder m_recorder;
};
}
