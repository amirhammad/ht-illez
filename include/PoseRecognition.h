/*
 * This file is part of the project HandTrackerApp - ht-illez
 * hosted at http://github.com/amirhammad/ht-illez
 *
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
 *
 *
 * HandTrackerApp - ht-illez is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once
#include "Types.h"

#include <opencv2/opencv.hpp>
#include <QObject>
#include <QVector>
#include <opennn.h>
#include <QMutex>
#include <QMetaType>
#include <QVariantList>

class QFile;

namespace iez {
class PoseResultAnalyzer;
class PoseRecognition {
public:
	enum POSE {
		POSE_0 = 0,
		POSE_1 = 1,
		POSE_2 = 2,
		POSE_3 = 3,
		POSE_4 = 4,
		POSE_5 = 5,
		POSE_6 = 6,
		POSE_7 = 7,
		POSE_8 = 8,
		POSE_9 = 9,
		POSE_10 = 10,
		POSE_11 = 11,

		POSE_END
	};

	PoseRecognition();
	~PoseRecognition();
	void learnNew(const POSE pose,
			   const cv::Point palmCenter,
			   const float palmRadius,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);
	void savePoseDatabase(QString path);
	void loadPoseDatabase(QString path);
	void neuralNetworkImport(QString path);
	void neuralNetworkLoad(QString path);
	void neuralNetworkSave(QString path);
	QString databaseToString() const;

	QVariantList categorize(const cv::Point palmCenter,
					const float palmRadius,
					const wristpair_t &wrist,
					const QList<cv::Point> &fingertips);

	int calculateOutput(OpenNN::Vector<double> featureVector) const;

	bool testNeuralNetwork() const;

	struct Data {
		Data(int size)
		: input(size) {

		}

		QVector<float> input;
		int output;
	};

	template <typename Container>
	static int findBestMatchIndex(Container container, double target)
	{
		int minIndex = 0;
		double minValue = std::numeric_limits<double>::max();

		for (int i = 0; i < container.size(); i++) {
			double err = qAbs(target - container[i]);
			if (minValue > err) {
				minValue = err;
				minIndex = i;
			}
		}

		return minIndex;
	}

	static QString poseToString(enum POSE pose);
	static QString poseToString(int pose);
	static int inputVectorSize();
	static double normalizeInto(double value, double low = 0.0, double high = 0.0);
	static QList<Data> loadDatabaseFromFile(QString path);
	static void saveDatabaseToFile(QString path, QList<Data> database);

	struct TrainArgs {
	uint hiddenNeuronCount;
	enum ActivationFunction {
		LINEAR,
		LOGISTIC_SIGMOID,
		HYPERBOLIC_TANGENT
	} activationFunction;
	};
	void train(TrainArgs args);

private:


	static bool loadLayer(QFile &fBias,
						  QFile &fWeights,
						  const int inputCount,
						  const int neuronCount,
						  OpenNN::PerceptronLayer &layer);
	static bool loadMLP(QString prefix, OpenNN::MultilayerPerceptron& mlp);




	QList<Data> m_database;

	static void normalizeVector(OpenNN::Vector<double> &vec);

	static OpenNN::Vector<double> constructFeatureVector(	const cv::Point palmCenter,
															const float palmRadius,
															const wristpair_t &wrist,
															const QList<cv::Point> &fingertips);

	static OpenNN::Matrix<double> convertToNormalizedMatrix(const QList<Data> &db);

	OpenNN::Matrix<double> m_matrix;
	OpenNN::NeuralNetwork *m_neuralNetwork;
	mutable QMutex m_nnMutex;
	mutable QMutex m_dbMutex;

	PoseResultAnalyzer *m_poseResultAnalyzer;
};

}
