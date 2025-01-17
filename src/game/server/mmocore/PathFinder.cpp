/* Pathfind class by Sushi */
#include "PathFinder.h"

#include <game/collision.h>
#include <game/layers.h>
#include <game/mapitems.h>

CPathFinder::CPathFinder(CLayers* Layers, CCollision* Collision) : m_pLayers(Layers), m_pCollision(Collision)
{
	m_ClosedNodes = 0;
	m_FinalSize = 0;
	m_StartIndex = -1;
	m_EndIndex = -1;

	m_LayerWidth = m_pLayers->GameLayer()->m_Width;
	m_LayerHeight = m_pLayers->GameLayer()->m_Height;

	// set size for array
	m_lMap.set_size(m_LayerWidth * m_LayerHeight);
	m_lNodes.set_size(m_LayerWidth * m_LayerHeight);
	m_lFinalPath.set_size(MAX_WAY_CALC);

	// init size for heap
	m_Open.SetSize(4 * MAX_WAY_CALC);

	int ID = 0;
	for (int i = 0; i < m_LayerHeight; i++)
	{
		for (int j = 0; j < m_LayerWidth; j++)
		{
			CNode Node;
			Node.m_Pos = vec2(j, i);
			Node.m_Parent = -1;
			Node.m_ID = ID;
			Node.m_G = 0;
			Node.m_H = 0;
			Node.m_F = 0;
			Node.m_IsClosed = false;
			Node.m_IsOpen = false;
			if (m_pCollision->CheckPoint(j * 32 + 16, i * 32 + 16))
				Node.m_IsCol = true;
			else
				Node.m_IsCol = false;

			// add the node to the list
			m_lMap[i * m_LayerWidth + j] = Node;
			ID++;
		}
	}

	// create handler
	m_pHandler = new CHandler(this);
}

CPathFinder::~CPathFinder()
{
	m_lMap.clear();
	m_lFinalPath.clear();
	m_lNodes.clear();

	// delete handler
	delete m_pHandler;
	m_pHandler = nullptr;
}

void CPathFinder::Init()
{
	m_ClosedNodes = 0;
	m_FinalSize = 0;
	m_Open.MakeEmpty();
	m_lNodes = m_lMap;
}

void CPathFinder::SetStart(vec2 Pos)
{
	int StartX = clamp((int)(Pos.x / 32.0f), 0, m_LayerWidth - 1);
	int StartY = clamp((int)(Pos.y / 32.0f), 0, m_LayerHeight - 1);

	int Index = GetIndex(StartX, StartY);
	m_lNodes[Index].m_Parent = START;
	m_lNodes[Index].m_IsClosed = true;
	m_ClosedNodes++;
	m_StartIndex = Index;
}

void CPathFinder::SetEnd(vec2 Pos)
{
	int EndX = clamp((int)(Pos.x / 32.0f), 0, m_LayerWidth - 1);
	int EndY = clamp((int)(Pos.y / 32.0f), 0, m_LayerHeight - 1);

	int Index = GetIndex(EndX, EndY);
	m_lNodes[Index].m_Parent = END;
	m_EndIndex = Index;
}


int CPathFinder::GetIndex(int XPos, int YPos) const
{
	return XPos + m_pLayers->GameLayer()->m_Width * YPos;
}

void CPathFinder::FindPath()
{
	int CurrentIndex = m_StartIndex;
	if (m_StartIndex > -1 && m_EndIndex > -1)
	{
		while (m_ClosedNodes < MAX_WAY_CALC && m_lNodes[CurrentIndex].m_ID != m_EndIndex)
		{
			for (int i = 0; i < 4; i++)
			{
				// get the working index
				int WorkingIndex = -1;

				switch (i)
				{
				case 0:
					if (CurrentIndex + 1 < m_lNodes.size())
						WorkingIndex = CurrentIndex + 1;
					break;
				case 1:
					if (CurrentIndex - 1 >= 0)
						WorkingIndex = CurrentIndex - 1;
					break;
				case 2:
					if (CurrentIndex + m_LayerWidth < m_lNodes.size())
						WorkingIndex = CurrentIndex + m_LayerWidth;
					break;
				case 3:
					if (CurrentIndex - m_LayerWidth >= 0)
						WorkingIndex = CurrentIndex - m_LayerWidth;
				}

				if (WorkingIndex > -1 && !m_lNodes[WorkingIndex].m_IsCol && !m_lNodes[WorkingIndex].m_IsClosed)
				{
					if (!m_lNodes[WorkingIndex].m_IsOpen)
					{
						// set its parent
						m_lNodes[WorkingIndex].m_Parent = CurrentIndex;

						// calculate the important values
						m_lNodes[WorkingIndex].m_G = m_lNodes[CurrentIndex].m_G + 1;
						m_lNodes[WorkingIndex].m_H = (abs((int)(m_lNodes[WorkingIndex].m_Pos.x - m_lNodes[m_EndIndex].m_Pos.x)) + abs((int)(m_lNodes[WorkingIndex].m_Pos.y - m_lNodes[m_EndIndex].m_Pos.y)));
						m_lNodes[WorkingIndex].m_F = m_lNodes[WorkingIndex].m_G + m_lNodes[WorkingIndex].m_H;

						m_lNodes[WorkingIndex].m_IsOpen = true;

						m_Open.Insert(m_lNodes[WorkingIndex]);
					}
					else
					{
						// recalculate the G and F Value
						if (m_lNodes[WorkingIndex].m_G > m_lNodes[CurrentIndex].m_G + 1)
						{
							// set new parent
							m_lNodes[WorkingIndex].m_Parent = CurrentIndex;

							// important values (H value wont change)
							m_lNodes[WorkingIndex].m_G = m_lNodes[CurrentIndex].m_G + 1;
							m_lNodes[WorkingIndex].m_F = m_lNodes[WorkingIndex].m_G + m_lNodes[WorkingIndex].m_H;

							m_Open.Replace(m_lNodes[WorkingIndex]);
						}
					}
				}
			}

			if (m_Open.GetSize() < 1)
				return;

			// get Lowest F from heap and set new CurrentIndex
			CurrentIndex = m_Open.GetMin()->m_ID;

			// delete it \o/
			m_Open.RemoveMin();

			// set the one with lowest score to closed list and begin new from there
			m_lNodes[CurrentIndex].m_IsClosed = true;
			m_ClosedNodes++;
		}

		// go backwards and return final path :-O
		while (m_lNodes[CurrentIndex].m_ID != m_StartIndex)
		{
			m_lFinalPath[m_FinalSize] = m_lNodes[CurrentIndex];
			m_FinalSize++;

			// get next node
			CurrentIndex = m_lNodes[CurrentIndex].m_Parent;
		}
	}
}

vec2 CPathFinder::GetRandomWaypoint()
{
	array<vec2> lPossibleWaypoints;
	for (int i = 0; i < m_LayerHeight; i++)
	{
		for (int j = 0; j < m_LayerWidth; j++)
		{
			if (m_lMap[i * m_LayerWidth + j].m_IsClosed || m_lMap[i * m_LayerWidth + j].m_IsCol)
				continue;

			lPossibleWaypoints.add(m_lMap[i * m_LayerWidth + j].m_Pos);
		}
	}

	if (lPossibleWaypoints.size())
	{
		int Rand = secure_rand() % lPossibleWaypoints.size();
		return lPossibleWaypoints[Rand];
	}
	return vec2(0, 0);
}

vec2 CPathFinder::GetRandomWaypointRadius(vec2 Pos, float Radius)
{
	array<vec2> lPossibleWaypoints;
	float Range = (Radius / 2.0f);
	int StartX = clamp((int)((Pos.x - Range) / 32.0f), 0, m_LayerWidth - 1);
	int StartY = clamp((int)((Pos.y - Range) / 32.0f), 0, m_LayerHeight - 1);
	int EndX = clamp((int)((Pos.x + Range) / 32.0f), 0, m_LayerWidth - 1);
	int EndY = clamp((int)((Pos.y + Range) / 32.0f), 0, m_LayerHeight - 1);

	for(int i = StartY; i < EndY; i++)
	{
		for(int j = StartX; j < EndX; j++)
		{
			if(m_lMap[i * m_LayerWidth + j].m_IsClosed || m_lMap[i * m_LayerWidth + j].m_IsCol)
				continue;

			lPossibleWaypoints.add(m_lMap[i * m_LayerWidth + j].m_Pos);
		}
	}

	if(lPossibleWaypoints.size())
	{
		int Rand = secure_rand() % lPossibleWaypoints.size();
		return lPossibleWaypoints[Rand];
	}
	return vec2(0, 0);
}

/*
 * CHandler
 * - Synchronized path search in a separate thread pool
 */

CPathFinderPrepared::CData CPathFinder::CHandler::CallbackFindPath(const std::shared_ptr<HandleArgsPack>& pHandleData)
{
	HandleArgsPack* pHandle = pHandleData.get();

	if(pHandle && pHandle->IsValid() && length(pHandle->m_StartFrom) > 0 && length(pHandle->m_Search) > 0)
	{
		// guard element, path finder working only with one item TODO: rework
		std::lock_guard QueueLock { pHandle->m_PathFinder->m_mtxLimitedOnceUse };

		const vec2 StartPos = pHandle->m_StartFrom;
		const vec2 SearchPos = pHandle->m_Search;
		CPathFinder* pPathFinder = pHandle->m_PathFinder;

		// path finder working
		pPathFinder->Init();
		pPathFinder->SetStart(StartPos);
		pPathFinder->SetEnd(SearchPos);
		pPathFinder->FindPath();

		// initilize for future data
		CPathFinderPrepared::CData Data;
		Data.m_Type = CPathFinderPrepared::TYPE::DEFAULT;
		Data.m_Points.reserve(pPathFinder->m_FinalSize);
		for(int i = pPathFinder->m_FinalSize - 1, j = 0; i >= 0; i--, j++)
		{
			Data.m_Points[j] = vec2(pPathFinder->m_lFinalPath[i].m_Pos.x * 32 + 16, pPathFinder->m_lFinalPath[i].m_Pos.y * 32 + 16);
		}

		return Data;
	}

	return {};
}

CPathFinderPrepared::CData CPathFinder::CHandler::CallbackRandomRadiusWaypoint(const std::shared_ptr<HandleArgsPack>& pHandleData)
{
	HandleArgsPack* pHandle = pHandleData.get();

	if(pHandle && pHandle->IsValid() && length(pHandle->m_StartFrom) > 0)
	{
		// guard element, path finder working only with one item TODO: rework
		std::lock_guard QueueLock { pHandle->m_PathFinder->m_mtxLimitedOnceUse };

		const vec2 StartPos = pHandle->m_StartFrom;

		// path finder working
		const vec2 TargetPos = pHandle->m_PathFinder->GetRandomWaypointRadius(StartPos, pHandle->m_Radius);

		// initilize for future data
		CPathFinderPrepared::CData Data;
		Data.m_Type = CPathFinderPrepared::TYPE::RANDOM;
		Data.m_Points[0] = vec2(TargetPos.x * 32, TargetPos.y * 32);
		return Data;
	}

	return{};
}

bool CPathFinder::CHandler::TryMarkAndUpdatePreparedData(CPathFinderPrepared* pPrepare, vec2* pTarget, vec2* pOldTarget)
{
	// check future status
	if(pPrepare && pPrepare->m_FutureData.valid() && pPrepare->m_FutureData.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
	{
		pPrepare->m_Data.Clear();
		pPrepare->m_Data = pPrepare->m_FutureData.get();
		pPrepare->m_Data.Prepare(pTarget, pOldTarget);
		pPrepare->m_FutureData = {};
		return true;
	}

	return false;
}
