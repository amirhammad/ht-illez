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


#include "PoseResultAnalyzer.h"

namespace iez {

PoseResultAnalyzer::PoseResultAnalyzer(int acceptableCount)
	: m_count(0)
	, m_poseId(POSE_UNDEF)
	, m_acceptableCount(acceptableCount)
	, m_candidatePoseId(POSE_UNDEF)
{

}

PoseResultAnalyzer::~PoseResultAnalyzer()
{

}

int PoseResultAnalyzer::feed(int poseId)
{
	if (m_poseId == poseId) {
		// everything is ok
	} else if (m_poseId != poseId) {
		if (m_candidatePoseId == POSE_UNDEF
		|| m_candidatePoseId != poseId) {
			m_candidatePoseId = poseId;
			m_count = 1;
		} else if (m_candidatePoseId == poseId) {
			m_count++;
		}

		if (m_count >= m_acceptableCount) {
			m_poseId = m_candidatePoseId;
			m_candidatePoseId = POSE_UNDEF;
			m_errors = 0;
		}

		if (m_errors < m_acceptableCount) {
			m_errors++;
		} else {
			m_poseId = POSE_UNDEF;
			m_errors = 0;
		}
	}

	return m_poseId;
}

int PoseResultAnalyzer::poseId() const
{
	return m_poseId;
}

}
