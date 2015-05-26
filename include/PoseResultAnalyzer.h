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
namespace iez {

class PoseResultAnalyzer {
public:
	explicit PoseResultAnalyzer(int acceptableCount);
	~PoseResultAnalyzer();

	int feed(int poseId);
	template <typename Container>
	static int findBestMatchIndex(const Container &container, double target = 1.0)
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

	int poseId() const;
	enum {POSE_UNDEF = -1};
private:
	int m_count;
	int m_poseId;
	int m_candidatePoseId;
	int m_errors;
	const int m_acceptableCount;
};

}
