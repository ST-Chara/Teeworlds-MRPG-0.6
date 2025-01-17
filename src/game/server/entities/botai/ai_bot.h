/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_BOTAI_CONTROLLER_HELPER_H
#define GAME_SERVER_BOTAI_CONTROLLER_HELPER_H

class CCharacterBotAI;

enum class TARGET_TYPE
{
	EMPTY,
	ACTIVE,
	LOST
};

class CAIController
{
	// target system
	class CTargetSystem
	{
		int m_TargetID { -1 };
		bool m_IsCollised {};
		int m_Aggression {};
		TARGET_TYPE m_Type {};
		CCharacterBotAI* m_pCharacter {};

	public:
		void Init(CCharacterBotAI* pCharacter)
		{
			m_pCharacter = pCharacter;
		}

		void Reset()
		{
			if(m_pCharacter)
			{
				m_TargetID = -1;
				m_Aggression = 0;
				m_IsCollised = false;
				m_Type = TARGET_TYPE::EMPTY;
			}
		}

		void Tick()
		{
			if(m_pCharacter)
			{
				if(m_Type == TARGET_TYPE::LOST && m_Aggression)
				{
					m_Aggression--;
					if(!m_Aggression)
						Reset();
				}
			}
		}
		int GetCID() const { return m_TargetID; }
		int GetAggresion() const { return m_Aggression; }
		void Set(int ClientID, int Aggression)
		{
			if(m_pCharacter && ClientID >= 0 && ClientID < MAX_CLIENTS)
			{
				m_TargetID = ClientID;
				m_Aggression = Aggression;
				m_Type = TARGET_TYPE::ACTIVE;
			}
		}

		void SetType(TARGET_TYPE TargetType)
		{
			if(m_pCharacter)
			{
				m_Type = TargetType;
			}
		}

		bool IsEmpty() const { return m_TargetID <= -1; }
		bool IsCollised() const { return m_IsCollised; }
		void UpdateCollised(bool Collised) { m_IsCollised = Collised; }
	};

public:
	// Constructor
	CAIController(CCharacterBotAI* pCharacter);

	// Destructor
	~CAIController(){}

	// Function to get the entity character
	CCharacterBotAI* GetCharacter() const { return m_pCharacter; };
	CTargetSystem* GetTarget() { return &m_Target; }

	// Function to process the action
	//void ProcessHandle();

private:
	// Pointer to the entity character
	CCharacterBotAI* m_pCharacter;
	CTargetSystem m_Target;
};

#endif
