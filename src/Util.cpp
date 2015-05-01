#include "Util.h"
#include <QObject>
namespace iez {
float Util::findMax2(const cv::Mat &mat, cv::Point &point)
{
	Q_ASSERT(mat.type() == CV_32FC1);

	point = cv::Point(-1, -1);
	float max = 0;
	for (int i = 0; i < mat.rows; i++) {
		for (int j = 0; j < mat.cols; j++) {
			const float d = mat.at<float>(i, j);
			if (d > max) {
				max = d;
				point = cv::Point(j, i);
			}
		}
	}
	return max;
}

void Util::filterDepth(cv::Mat &dst, const cv::Mat &depth, int near, int far)
{
	Q_ASSERT(dst.rows == depth.rows);
	Q_ASSERT(dst.cols == depth.cols);

	if (near == -1) {
		near = findMin(depth);
		far = near + 170;
	}
	const cv::Mat &mask = filterDepthMask(depth, near, far);
//	WindowManager::getInstance()->imShow("x", mask);
	cv::Mat dst2;

	dst.copyTo(dst2, mask);
//	WindowManager::getInstance()->imShow("x2", dst2);
	dst2.copyTo(dst);
}

cv::Mat Util::filterDepthMask(const cv::Mat &depth, int near, int far)
{
	cv::Mat outputMask;
	outputMask.create(depth.rows, depth.cols, CV_8UC1);

	if (near == -1) {
		near = findMin(depth);
		far = near + 150;
	}

	for (int y = 0; y < depth.rows; ++y) {
		for (int x = 0; x < depth.cols; ++x) {
			const uint16_t val = depth.at<uint16_t>(y, x);

			if (val > near && val < far) {
				outputMask.at<uint8_t>(y, x) = 255;
			} else {
				outputMask.at<uint8_t>(y, x) = 0;
			}
		}
	}

	return outputMask;
}


cv::Point Util::calculateMean(const std::vector<cv::Point> &pointVector)
{
	if (pointVector.size() == 0) {
		return cv::Point(0,0);
	}

	cv::Point mean(0,0);

	for (int i = 0; i < pointVector.size(); i++) {
		mean = mean + pointVector[i];
	}
	int size = pointVector.size();
	return cv::Point(mean.x/size, mean.y/size);
}

cv::Point Util::findNearestPoint(const std::vector<cv::Point> &pointVector, const cv::Point refPoint)
{
	float minDistance = std::numeric_limits<float>::max();
	if (pointVector.size() == 0) {
		return cv::Point();
	}
	cv::Point nearestPoint = pointVector[0];
	for (int i = 1; i < pointVector.size(); i++) {
		float d = pointDistance(refPoint, pointVector[i]);
		if (minDistance > d) {
			minDistance = d;
			nearestPoint = pointVector[i];
		}
	}
	return nearestPoint;
}

/**
 * Rotate an image
 */
void Util::rotate(cv::Mat& src, double angle, cv::Mat& dst)
{
	int len = std::max(src.cols, src.rows);
	cv::Point2f pt(len/2., len/2.);
	cv::Mat r = cv::getRotationMatrix2D(pt, angle, 1.0);

	cv::warpAffine(src, dst, r, cv::Size(len, len), cv::INTER_NEAREST);
}



int Util::findMin(const cv::Mat& depth)
{
	cv::Point dummy;
	return findMin2(depth, dummy);
}

int Util::findMin2(const cv::Mat& depth, cv::Point &point)
{
	int minDepth = 10000;
	for (int i = 0; i < depth.rows; i++) {
		for (int j = 0; j < depth.cols; j++) {
			const int d = depth.at<uint16_t>(i, j);
			if (d > 0) {
				if (minDepth > d) {
					minDepth = d;
					point = cv::Point(j, i);
				}
			}
		}
	}

	return minDepth;
}

float Util::findMax(const cv::Mat &mat)
{
	cv::Point dummy;
	return findMax2(mat, dummy);
}

float Util::pointDistance(const cv::Point& pt1, const cv::Point& pt2)
{
	const cv::Point diff = pt1 - pt2;
	return sqrtf(diff.dot(diff));
}

cv::Point Util::pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12)
{
	return cv::Point((pt1.x*ratio12 + pt2.x*(1 - ratio12)), (pt1.y*ratio12 + pt2.y*(1 - ratio12)));
}


cv::Point Util::calculateMeanIndices(const cv::Mat &mat)
{
	if (mat.cols*mat.rows == 0) {
		return cv::Point(0,0);
	}

	cv::Point total(0,0);
	int wsum = 0;
	for (int i = 0; i < mat.rows; i++) {
		for (int j = 0; j < mat.cols; j++) {
			if (mat.at<uint8_t>(i,j)) {
				total = total + cv::Point(j,i);
				wsum++;
			}
		}
	}
	return cv::Point(total.x/wsum, total.y/wsum);
}

cv::Point Util::calculateWeightedMean(const std::vector<cv::Point> &pointVector)
{
	if (pointVector.size() == 0) {
		return cv::Point(0,0);
	}

	cv::Point2f mean(0,0);
	float wsum = 0;
	for (int i = 0; i < pointVector.size(); i++) {
		cv::Point diffPrev = pointVector[i] - pointVector[i?i-1:pointVector.size()-1];
		cv::Point diffNext = pointVector[i] - pointVector[(i==pointVector.size()-1)?i+1:0];

		float weight = sqrt(diffPrev.dot(diffPrev)) + sqrt(diffNext.dot(diffNext));
		qDebug("%+7.3f", weight);
		mean = mean + cv::Point2f(pointVector[i].x*weight, pointVector[i].y*weight);
		wsum += weight;
	}
	return cv::Point(mean.x/wsum, mean.y/wsum);
}

}
