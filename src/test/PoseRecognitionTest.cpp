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

#include "../PoseRecognition.cpp"
#include "Util.h"
using namespace iez;
	cv::Point Util::pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12)
	{
		return cv::Point((pt1.x*ratio12 + pt2.x*(1 - ratio12)), (pt1.y*ratio12 + pt2.y*(1 - ratio12)));
	}

#include <gtest/gtest.h>

class PoseRecognitionTest : public testing::Test
{
	void SetUp() {
		srand(time(NULL));
	}

	void TearDown() {

	}


protected:
	PoseRecognition *pose;
};



TEST_F(PoseRecognitionTest, integrityTest)
{
	int j = 100;
	QList<PoseRecognition::Data> db;

	for (int m = 0; m < j; m++) {
		PoseRecognition::Data dataSource(NN_INPUT_VECTOR_SIZE);
		for (int i = 0; i < NN_INPUT_VECTOR_SIZE; i++) {
			dataSource.input[i] = 100000.0f/rand()*((rand()%2) ? -1 : 1);
		}
		dataSource.output = rand()%100;

		db.append(dataSource);
	}

	PoseRecognition::saveDatabaseToFile("/tmp/k.csv", db);
	QList<PoseRecognition::Data> dbResult = PoseRecognition::loadDatabaseFromFile("/tmp/k.csv");

	EXPECT_EQ(dbResult.size(), db.size());
	for (int m = 0; m < j; m++) {
		for (int i = 0; i < NN_INPUT_VECTOR_SIZE; i++) {
			EXPECT_FLOAT_EQ(QString::number(db[m].input[i]).toFloat(), QString::number(dbResult[m].input[i]).toFloat());
		}
		EXPECT_EQ(db[m].output, dbResult[m].output);
	}


}

