class BGLayer6 extends BGBaseLayer // Battle of Tisy
{
    vector m_EventPosition;
    vector m_PreviousEventPosition;
    bool m_PreviousEventPositionSet;
    string m_CurrentLocationName, m_LastLocationName, m_SecondLastLocationName;
    protected ref array<Object> m_CurrentObjects = new array<Object> ();
	private bool m_LastCheckedPlayerInZone = false;
    protected float m_CaptureProgress = 0.0;
    private float m_CaptureProgressSpeed = 100.0 / 360.0; // % divided by seconds = 6 mins
    const float CAPTURE_PROGRESS_CHECK_INTERVAL = 1.0;
    const float PLAYERS_IN_RADIUS_CHECK_INTERVAL = 10.0;
    private int m_Direction = 1;

    ref array<string> m_playersWithin25m;
    ref array<string> m_playersWithin50m;
    ref array<string> m_playersWithin75m;

    override void InitLayer()
    {
        super.InitLayer();

        m_playersWithin25m = new array<string>();
        m_playersWithin50m = new array<string>();
        m_playersWithin75m = new array<string>();

        InitLocations();
        BGCreateNewEvent();
    }

    void BGCreateNewEvent()
    {
        BattlegroundsLocation location = GetNextLocation();
        if (location)
        {
            m_SecondLastLocationName = m_LastLocationName;
            m_LastLocationName = m_CurrentLocationName;

            m_EventPosition = location.GetPosition();
            SetCurrentLocationString(location.GetName());
            CreateSmokeEvent(m_EventPosition, "BGSmokeGreen", "BGSmokeRed", 100.0);
                
            m_CaptureProgress = 0.0;
            m_PreviousEventPosition = m_EventPosition;
            m_PreviousEventPositionSet = true;

            Print("[BattlegroundsV6] New Event: " + GetCurrentLocationName());
        }

        SetTeleporterDestinations();
    }

    override void SetTeleporterDestinations() 
    {
        if (m_Teleporters && m_Teleporters.Count() > 0)
        {
            vector destination = m_EventPosition;
            for (int i = 0; i < m_Teleporters.Count(); i++)
            {
                m_Teleporters[i].SetDestination(destination);
            }
        }
    }

	override void CheckCaptureProgress()
    {
        array<Man> playersInRadius = GetPlayersInRadius(m_EventLocation, 75, false);
        array<Man> validPlayers = new array<Man>;

        for (int i = 0; i < playersInRadius.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(playersInRadius[i]);
            if (player && player.IsAlive() && !player.IsPlayerDisconnected())
            {
                validPlayers.Insert(player);
            }
        }
        bool isPlayerInZone = (validPlayers.Count() > 0);

        if (m_CaptureProgress >= 100.0 || (!isPlayerInZone && m_CaptureProgress <= 0.0))
        {
            return;
        }

        if (isPlayerInZone)
        {
            m_CaptureProgress += m_CaptureProgressSpeed * CAPTURE_PROGRESS_CHECK_INTERVAL;
        }
        else
        {
            m_CaptureProgress -= m_CaptureProgressSpeed * CAPTURE_PROGRESS_CHECK_INTERVAL;
        }

        m_CaptureProgress = Math.Clamp(m_CaptureProgress, 0.0, 100.0);

        if (isPlayerInZone)
        {
            //Print("[BGLayer6] Player in zone. Progress: " + m_CaptureProgress.ToString());
        }
        else
        {
            //Print("[BGLayer6] No players in zone. Progress: " + m_CaptureProgress.ToString());
        }

        if (m_CaptureProgress == 100.0)
        {
            OnCaptureComplete();
            BGCreateNewEvent();
        }

        m_LastCheckedPlayerInZone = isPlayerInZone;
    }

    override void CheckCaptureRadius()
    {
        m_playersWithin25m.Clear();
        m_playersWithin50m.Clear();
        m_playersWithin75m.Clear();

        array<Man> playersInRadius = GetPlayersInRadius(m_EventLocation, 75, true);
        string currencyClass = "TraderPlus_Money_Dollar1";

        for (int i = 0; i < playersInRadius.Count(); i++)
        {
            Man playerMan = playersInRadius[i];
            PlayerBase player = PlayerBase.Cast(playerMan);
            if (!player || !player.IsAlive() || player.IsPlayerDisconnected())
                continue;

            float distanceSqToEvent = vector.DistanceSq(player.GetPosition(), m_EventLocation);
            string playerName = player.GetIdentity().GetName();
            int currencyAmount;

            if (distanceSqToEvent <= 625.0) // 25m x 25m = 625m sq
            {
                m_playersWithin25m.Insert(playerName);
                currencyAmount = 100;
            }
            else if (distanceSqToEvent <= 2500.0)
            {
                m_playersWithin50m.Insert(playerName);
                currencyAmount = 50;
            }
            else
            {
                m_playersWithin75m.Insert(playerName);
                currencyAmount = 20;
            }

            m_CurrencyHelper.AddCurrencyStackToPlayer(player, currencyClass, currencyAmount);
            
            if (rewardManager)
            {
                rewardManager.UpdatePlayerStat(player, "cashEarned", currencyAmount);
            }
            else
            {
                //Print("[BGLayer6] rewardManager is not initialized");
            }
        }
    }

	override void OnCaptureComplete()
    {
        super.OnCaptureComplete();

        if (m_LastRewardCrate)
        {
            V1Rewards.AddRewardLoot(m_LastRewardCrate);
        }

        else
        {
            //Print("[BGLayer6] Last reward crate not found");
        }
    }

    BattlegroundsLocation GetNextLocation()
    {
        if (m_EventLocations.Count() == 0)
        {
            return null;
        }

        // No previous event, start at Headquarters
        if (!m_PreviousEventPositionSet)
        {
            BattlegroundsLocation bridgeLocation = GetLocationByName("Headquarters [Tisy Military]");
            m_CurrentLocationString = bridgeLocation.GetName();  // Set the current location string
            if (Math.RandomInt(0, 2) == 0)
            {
                m_Direction = 1;
            }
            else
            {
                m_Direction = -1;
            }
            return bridgeLocation;
        }
        
        int currentIndex = -1;

        // Index the current location
        for (int i = 0; i < m_EventLocations.Count(); i++)
        {
            if (m_EventLocations[i].GetName() == m_CurrentLocationName)
            {
                currentIndex = i;
                break;
            }
        }

        if (currentIndex == -1)
        {
            return null;
        }

        // If at the start or end of the array, return to Headquarters
        if (currentIndex == 0 || currentIndex == m_EventLocations.Count() - 1)
        {
            BattlegroundsLocation supermarketLocation = GetLocationByName("Headquarters [Tisy Military]");
            m_CurrentLocationString = supermarketLocation.GetName();
            m_Direction = -m_Direction; // Reverse direction after visiting the Headquarters
            return supermarketLocation;
        }

        int nextIndex = currentIndex + m_Direction;

        BattlegroundsLocation nextLocation = m_EventLocations[nextIndex];
        m_CurrentLocationString = nextLocation.GetName();  // Set the current location string

        return nextLocation;
    }

    BattlegroundsLocation GetLocationByName(string name)
    {
        for (int i = 0; i < m_EventLocations.Count(); i++)
        {
            if (m_EventLocations[i].GetName() == name)
                return m_EventLocations[i];
        }
        return null;
    }

    override void SetCurrentLocationString(string locationName)
    {
        super.SetCurrentLocationString(locationName);
        m_CurrentLocationName = locationName;
    }

    string GetCurrentLocationName()
    {
        return m_CurrentLocationName;
    }

    override float GetCaptureRadius()
    {
        return 75.0;
    }

    override float GetCaptureProgress()
    {
        return m_CaptureProgress;
    }

    string GetPlayersListString(array<string> players)
    {
        string playersList = "";
        for (int i = 0; i < players.Count(); i++)
        {
            if (i != 0)
            {
                playersList += ", ";
            }

            playersList += players[i];
        }

        return playersList;
    }

	void InitLocations()
    {
        m_EventLocations.Insert(new BattlegroundsLocation("1344.592285 455.445648 14477.800781", "Shooting Range [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1504.857178 460.350677 14441.251953", "North Barrack [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1652.532349 451.809814 14360.707031", "Barracks [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1642.326294 451.742401 14199.715820", "Headquarters [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1570.156128 452.782440 14053.088867", "Garages [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1674.504883 448.241333 13923.373047", "South Barrack [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1738.978149 451.732422 13734.559570", "West Helipad [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1640.206177 437.098389 13581.860352", "East Tents [Tisy Military]"));
        m_EventLocations.Insert(new BattlegroundsLocation("1500.699951 440.539886 13642.720703", "West Tents [Tisy Military]"));
    }
}