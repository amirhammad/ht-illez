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

#include "../PoseResultAnalyzer.cpp"
using namespace iez;

#include <gtest/gtest.h>

class PoseResultAnalyzerTest : public testing::Test
{
	void SetUp() {
		srand(time(0));
	}


	void TearDown() {

	}


protected:

};

TEST_F(PoseResultAnalyzerTest, function)
{
	const int acceptableCount = 10;
	const int pose1 = 5;
	const int pose2 = 2;
	PoseResultAnalyzer analyzer(acceptableCount);

	for (int i = 0; i < acceptableCount - 1; i++) {
		analyzer.feed(pose1);
		EXPECT_EQ(analyzer.poseId(), PoseResultAnalyzer::POSE_UNDEF);
	}
	analyzer.feed(pose1);
	EXPECT_EQ(analyzer.poseId(), pose1);


	for (int i = 0; i < acceptableCount - 1; i++) {
		analyzer.feed(pose2);
		EXPECT_EQ(analyzer.poseId(), pose1);
	}
	analyzer.feed(pose2);
	EXPECT_EQ(analyzer.poseId(), pose2);

	int index = 0;
	for (index = 0; index < acceptableCount - 1; index++) {
		int r = rand()%10;
		analyzer.feed((r == pose2) ? r/2 : r);
		EXPECT_EQ(analyzer.poseId(), pose2);
	}

	analyzer.feed(100);
	EXPECT_EQ(analyzer.poseId(), PoseResultAnalyzer::POSE_UNDEF);

}



