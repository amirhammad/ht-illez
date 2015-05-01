#include "ImageSource.h"

#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QObject>

namespace iez {
class ImageSourceArtificial;

class ImageSourceArtificial : public ImageSource {
	Q_OBJECT
public:
	ImageSourceArtificial();
	virtual ~ImageSourceArtificial();
	cv::Mat getColorMat() const;
	cv::Mat getDepthMat() const;
	void setColorMat(const cv::Mat &src);
	void setDepthMat(const cv::Mat &src);

	static ImageSourceArtificial *globalInstance();
public slots:
	void pause(bool p = true);
private:
	cv::Mat m_color;
	cv::Mat m_depth;
	mutable QMutex m_mutex;
};

}
