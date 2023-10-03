class BGLayer7 extends BGBaseLayer // Battle of Krasnostav
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

    ref array<string> m_playersWithin20m;
    ref array<string> m_playersWithin50m;
    ref array<string> m_playersWithin100m;

    override void InitLayer()
    {
        super.InitLayer();

        m_playersWithin20m = new array<string>();
        m_playersWithin50m = new array<string>();
        m_playersWithin100m = new array<string>();

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

            Print("[BattlegroundsV7] New Event: " + GetCurrentLocationName());
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
        array<Man> playersInRadius = GetPlayersInRadius(m_EventLocation, 100, false);
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
            //Print("[BGLayer7] Player in zone. Progress: " + m_CaptureProgress.ToString());
        }
        else
        {
            //Print("[BGLayer7] No players in zone. Progress: " + m_CaptureProgress.ToString());
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
        m_playersWithin20m.Clear();
        m_playersWithin50m.Clear();
        m_playersWithin100m.Clear();

        array<Man> playersInRadius = GetPlayersInRadius(m_EventLocation, 100, true);
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

            if (distanceSqToEvent <= 400.0) // 20m x 20m = 400m sq
            {
                m_playersWithin20m.Insert(playerName);
                currencyAmount = 100;
            }
            else if (distanceSqToEvent <= 2500.0)
            {
                m_playersWithin50m.Insert(playerName);
                currencyAmount = 50;
            }
            else
            {
                m_playersWithin100m.Insert(playerName);
                currencyAmount = 20;
            }

            m_CurrencyHelper.AddCurrencyStackToPlayer(player, currencyClass, currencyAmount);
            
            if (rewardManager)
            {
                rewardManager.UpdatePlayerStat(player, "cashEarned", currencyAmount);
            }
            else
            {
                //Print("[BGLayer7] rewardManager is not initialized");
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
            //Print("[BGLayer7] Last reward crate not found");
        }
    }

    BattlegroundsLocation GetNextLocation()
    {
        if (m_EventLocations.Count() == 0)
        {
            return null;
        }

        // No previous event, start at Supermarket
        if (!m_PreviousEventPositionSet)
        {
            BattlegroundsLocation bridgeLocation = GetLocationByName("Supermarket [Krasnostav]");
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

        // If at the start or end of the array, return to supermarket
        if (currentIndex == 0 || currentIndex == m_EventLocations.Count() - 1)
        {
            BattlegroundsLocation supermarketLocation = GetLocationByName("Supermarket [Krasnostav]");
            m_CurrentLocationString = supermarketLocation.GetName();
            m_Direction = -m_Direction; // Reverse direction after visiting the supermarket
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
        return 100.0;
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
		m_EventLocations.Insert(new BattlegroundsLocation("10135.995117 345.729614 12050.285156", "Zolotar West [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("10241.216797 366.627197 12015.442383", "Zolotar Castle [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("10322.644531 359.806732 12085.861328", "Zolotar North [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("10617.055664 264.782990 12320.417969", "Westside [Krasnostav]"));
		m_EventLocations.Insert(new BattlegroundsLocation("10951.153320 225.279526 12278.002930", "Church [Krasnostav]"));
		m_EventLocations.Insert(new BattlegroundsLocation("11119.563477 199.274948 12291.416992", "Supermarket [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("11276.066406 193.728485 12245.919922", "Workshop [Krasnostav]"));
		m_EventLocations.Insert(new BattlegroundsLocation("11551.574219 150.828476 12409.550781", "Eastside [Krasnostav]"));
		m_EventLocations.Insert(new BattlegroundsLocation("11869.314453 140.134811 12433.161133", "Airfield Hangar [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("11873.050781 140.012482 12556.015625", "Airfield Medical [Krasnostav]"));
        m_EventLocations.Insert(new BattlegroundsLocation("11980.098633 139.999985 12517.483398", "Airfield ATC [Krasnostav]"));
    }
}