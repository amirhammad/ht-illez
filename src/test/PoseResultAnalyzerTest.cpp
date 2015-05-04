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



