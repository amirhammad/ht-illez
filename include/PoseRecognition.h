/*
 * This file is part of the project HandTrackerApp - ht-illez
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

class PoseRecognition {
public:
	PoseRecognition();
	~PoseRecognition();

	void poseDatabaseAppend(const int pose,
			   const cv::Point palmCenter,
			   const float palmRadius,
			   const wristpair_t &wrist,
			   const QList<cv::Point> &fingertips);

	void poseDatabaseSave(QString path) const;
	void poseDatabaseLoad(QString path);
	void neuralNetworkImport(QString path);
	void neuralNetworkLoad(QString path);
	void neuralNetworkSave(QString path) const;
	bool neuralNetworkTest(float precision) const;
	class TrainArgs;
	void neuralNetworkTrain(TrainArgs args);

	QString databaseToString() const;

	QVariantList categorize(const cv::Point palmCenter,
					const float palmRadius,
					const wristpair_t &wrist,
					const QList<cv::Point> &fingertips);

	struct Data {
		Data(int size)
		: input(size)
		, output(-1) {

		}

		QVector<float> input;
		int output;
	};

	static QString poseToString(const int pose);
	static int inputVectorSize();
	static QList<Data> loadDatabaseFromFile(QString path);
	static void saveDatabaseToFile(QString path, QList<Data> database);

	class TrainArgs {
	public:
		uint hiddenNeuronCount;
		enum ActivationFunction {
			LINEAR,
			LOGISTIC_SIGMOID,
			HYPERBOLIC_TANGENT
		} activationFunction;
	};

    static QVector<double> constructFeatureQVector(	const cv::Point palmCenter,
                                                    const float palmRadius,
                                                    const wristpair_t &wrist,
                                                    const QList<cv::Point> &fingertips);
	int poseCount() const;
private:
	void checkMatrix(int minOccurence) const;
	int calculateOutput(OpenNN::Vector<double> featureVector) const;

	static bool loadLayer(QFile &fBias,
						  QFile &fWeights,
						  const int inputCount,
						  const int neuronCount,
						  OpenNN::PerceptronLayer &layer);
	static bool loadMLP(QString prefix, OpenNN::MultilayerPerceptron& mlp);



	static OpenNN::Vector<double> constructFeatureVector(	const cv::Point palmCenter,
															const float palmRadius,
															const wristpair_t &wrist,
															const QList<cv::Point> &fingertips);

	OpenNN::Matrix<double> convertToMatrix(const QList<Data> &db);

	QList<Data> m_database;
	OpenNN::Matrix<double> m_matrix;

	OpenNN::NeuralNetwork *m_neuralNetwork;

	mutable QMutex m_nnMutex;
	mutable QMutex m_dbMutex;

	uint m_poseCount;
};

}
