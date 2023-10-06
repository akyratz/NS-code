#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <arpa/inet.h>

using namespace ns3;

static void
 CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
 {
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
 }


class MyApp : public Application {
    private:
        Ptr<Socket> m_socket;
        Address m_peer;
        uint32_t m_packetSize;
        uint32_t m_nPackets;
        uint32_t m_interval;
        EventId m_sendEvent;
        bool m_running;
        uint32_t m_packetsSent;

        void ScheduleTx(void) {
            if(m_running){
                Time tNext(MilliSeconds(m_interval));
                m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
            }
        }

        void SendPacket(void) {
            Ptr<Packet> packet = Create<Packet>(m_packetSize);
            m_socket->Send(packet);

            if(++m_packetsSent < m_nPackets) {
                ScheduleTx();
            }
        }

        void Start() {
            m_running = true;
            m_packetsSent = 0;
            m_socket->Bind();
            m_socket->Connect(m_peer);
            SendPacket();
        }

        void Stop() {
            m_running = false;
            if(m_sendEvent.IsRunning()) {
                Simulator::Cancel(m_sendEvent);
            }
            if(m_socket) {
                m_socket->Close();
            }
        }

    public:
        MyApp()
        :   m_socket(0),
            m_peer(),
            m_packetSize(0),
            m_nPackets(0),
            m_interval(0),
            m_sendEvent(),
            m_running(false),
            m_packetsSent(0)
        {
        }

        virtual ~MyApp() {
            m_socket = 0;
        }

        void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, uint32_t interval) {
            m_socket = socket;
            m_peer = address;
            m_packetSize = packetSize;
            m_nPackets = nPackets;
            m_interval = interval;
        }

        void StartApplication(Time time) {
            Simulator::Schedule(time, &MyApp::Start, this);
        }

        void StopApplication(Time time) {
            Simulator::Schedule(time, &MyApp::Stop, this);
        }

        
        bool IsRunning() {
            return m_running;
        }

        Ipv4Address GetDstnIpAddr() {
            InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(m_peer);
            return iaddr.GetIpv4();
        }

        Ptr<Socket> GetSocket() {
            return m_socket;
        }
};


int main(int argc, char **argv) {

	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(18100));
	Config::SetDefault("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue(100));
	Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(26));
	Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));
	Config::SetDefault("ns3::LteUePhy::NoiseFigure", DoubleValue(9));
	Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(46));
	Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5));
	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
	Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(16777216));
    Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(2));

	uint16_t numOfUsers_1 = 10;
	uint16_t numOfUsers_2 = 2;
	uint16_t numofCells = 2;
    Vector enbPosition_1 = Vector(0, 1000, 30);
    Vector enbPosition_2 = Vector(1000, 0, 30);
    int ulPacketInterval = 100, dlPacketInterval = 1000; //ms
    int ulPacketSize = 300, dlPacketSize = 1000; //bytes
	double duration = 35;

	// Parse command line attribute
	CommandLine cmd;

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();
	//parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	//Create ue and enb nodes
	NodeContainer ueNodes_1, ueNodes_2, enbNodes;
	ueNodes_1.Create(numOfUsers_1);
	ueNodes_2.Create(numOfUsers_2);
	enbNodes.Create(numofCells);

	//Create LTE and EPC helpers
	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper>();
	lteHelper->SetEpcHelper(epcHelper);
	Ptr<Node> pgw = epcHelper->GetPgwNode(); //P-GW node
	epcHelper->SetAttribute("S1uLinkDelay", TimeValue(Seconds(0.01)));
	epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue(DataRate("100Mb/s")));
//	epcHelper->SetAttribute("S1uLinkEnablePcap", BooleanValue(true));

	//Set LTE parameters
//	//L=128.1+37.6*log10(R) R in km, L=15.3+37.6*log10(R) R in m
//	lteHelper->SetAttribute("PathlossModel", StringValue("ns3::LogDistancePropagationLossModel"));
//	lteHelper->SetPathlossModelAttribute("Exponent", DoubleValue(3.76));
//	lteHelper->SetPathlossModelAttribute("ReferenceLoss", DoubleValue(15.3));

	//Set LTE parameters
	//L=103.4+24.2*log10(R) R in km, L=30.8+24.2*log10(R) R in m, 3GPP TR 36.814 LOS
	lteHelper->SetAttribute("PathlossModel", StringValue("ns3::LogDistancePropagationLossModel"));
	lteHelper->SetPathlossModelAttribute("Exponent", DoubleValue(2.42));
	lteHelper->SetPathlossModelAttribute("ReferenceLoss", DoubleValue(30.8));

	// Set Wireless Fading
	lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
	lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("fading_trace_EPA_3kmph.fad"));

	//Set Scheduler Type
    lteHelper->SetAttribute("Scheduler", StringValue("ns3::FdBetFfMacScheduler"));

    //Set Antenna Type
    lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");

	//Create remote server
	NodeContainer remoteServerContainer;
	remoteServerContainer.Create(1);
	Ptr<Node> remoteServer = remoteServerContainer.Get(0);
	InternetStackHelper internet;
	internet.Install(remoteServerContainer); //install Internet stack on remote host

	// Create the Internet
    PointToPointHelper remoteServerp2p;//point to point line between PGW and remote server
	remoteServerp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("10Gb/s")));
	remoteServerp2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
	remoteServerp2p.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
	remoteServerp2p.EnablePcapAll("tcp_optimization_2", BooleanValue(true));

    NetDeviceContainer internetDevices;
    internetDevices = remoteServerp2p.Install(remoteServer, pgw);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.255.255.252");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteServerAddr = internetIpIfaces.GetAddress (0);
    Ipv4Address pgwServerAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteServerStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteServer->GetObject<Ipv4> ());
    remoteServerStaticRouting->SetDefaultRoute(pgwServerAddr, 1);

    //Set mobility for enb nodes and user nodes
    MobilityHelper enbmobility_1, enbmobility_2;
    Ptr<ListPositionAllocator> positionAlloc_1 = CreateObject<ListPositionAllocator> ();
    positionAlloc_1->Add (enbPosition_1);
    enbmobility_1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    enbmobility_1.SetPositionAllocator (positionAlloc_1);
    enbmobility_1.Install(enbNodes.Get(0));

    Ptr<ListPositionAllocator> positionAlloc_2 = CreateObject<ListPositionAllocator> ();
    positionAlloc_2->Add (enbPosition_2);
    enbmobility_2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    enbmobility_2.SetPositionAllocator (positionAlloc_2);
    enbmobility_2.Install(enbNodes.Get(1));


	MobilityHelper userMobility_1;
	userMobility_1.SetPositionAllocator("ns3::UniformDiscPositionAllocator", "rho", DoubleValue(400), "X", DoubleValue(0),"Y", DoubleValue(1000));
	userMobility_1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	userMobility_1.Install(ueNodes_1);

	MobilityHelper userMobility_2;
	userMobility_2.SetPositionAllocator("ns3::UniformDiscPositionAllocator", "rho", DoubleValue(400),"X", DoubleValue(1000), "Y", DoubleValue(0));
	userMobility_2.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	userMobility_2.Install(ueNodes_2);

//    for(uint32_t i = 0; i < userNodes.GetN(); ++i) {
//    	Ptr<LteUeNetDevice> userLte = userLteDevs.Get(i)->GetObject<LteUeNetDevice>();
//        Ptr<LteUePhy> userPhy = userLte->GetPhy();
//        userPhy->SetAttribute("TxPower", DoubleValue(26+5));
//    }

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer userLteDevs_1, userLteDevs_2;
    userLteDevs_1 = lteHelper->InstallUeDevice(ueNodes_1);
    userLteDevs_2 = lteHelper->InstallUeDevice(ueNodes_2);

    // Install the IP stack on the UEs
    internet.Install(ueNodes_1);
    internet.Install(ueNodes_2);
    Ipv4InterfaceContainer userIpIfaces_1, userIpIfaces_2;
    userIpIfaces_1 = epcHelper->AssignUeIpv4Address(NetDeviceContainer(userLteDevs_1));
    userIpIfaces_2 = epcHelper->AssignUeIpv4Address(NetDeviceContainer(userLteDevs_2));

    for(uint32_t i = 0; i < ueNodes_1.GetN(); ++i) {
    	Ptr<Node> userNode = ueNodes_1.Get(i);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(userNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    for(uint32_t i = 0; i < ueNodes_2.GetN(); ++i) {
      	Ptr<Node> userNode = ueNodes_2.Get(i);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(userNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    //Attach ueNodes to enbNodes
    lteHelper->Attach (userLteDevs_1, enbLteDevs.Get (0));
    lteHelper->Attach (userLteDevs_2, enbLteDevs.Get (1));


    //Create "ftp" apps
    ApplicationContainer ftpApps, sinkApps;
    uint16_t dlPort_1 = 1000;//downlink port for users of cell no1
    uint16_t dlPort_2 = 2000;//downlink port for users of cell no2
    uint32_t maxBytes = 10e6; //file size 10MB
    uint32_t packetSize = 1000; //bytes
    uint32_t Mss = 1440;
    uint32_t cwnd_1 = 3;
    uint32_t cwnd_2 = 3;


    //Uniform Random Variable for Start Times
    double min = 0.0;
    double max = 5.0;
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (min));
    x->SetAttribute ("Max", DoubleValue (max));

    for(uint16_t i = 0; i < ueNodes_1.GetN(); i++) {
    	//Packet sink(receiver) for ue
    	PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort_1));
    	ApplicationContainer sink = dlPacketSinkHelper.Install(ueNodes_1.Get(i)); //Add downlink sink to each user
    	sinkApps.Add(sink.Get(0)); //Add this sink to sinkApps Container

        //Downlink application (server sender)
        Ptr<Socket> serverSocket = Socket::CreateSocket(remoteServer, TcpSocketFactory::GetTypeId());
        serverSocket->SetInitialCwnd(cwnd_1);
        Ptr<MyApp> serverSender = CreateObject<MyApp>();
        Address downlinkAddress(InetSocketAddress(userIpIfaces_1.GetAddress(i), dlPort));
        serverSender->Setup(serverSocket, downlinkAddress, dlPacketSize, 1000000, dlPacketInterval);
        remoteServer->AddApplication(serverSender);
        ftpApps.Add(serverSender);

        serverSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
    	remoteServer->StartApplication(Seconds(x->GetValue()));
    }

    for(uint16_t i = 0; i < ueNodes_2.GetN(); i++) {
       	//Packet sink(receiver) for ue
       	PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort_2));
       	ApplicationContainer sink = dlPacketSinkHelper.Install(ueNodes_2.Get(i)); //Add downlink sink to each user
       	sinkApps.Add(sink.Get(0)); //Add this sink to sinkApps Container

        //Downlink application (server sender)
        Ptr<Socket> serverSocket = Socket::CreateSocket(remoteServer, TcpSocketFactory::GetTypeId());
        serverSocket->SetInitialCwnd(cwnd_2);
        Ptr<MyApp> serverSender = CreateObject<MyApp>();
        Address downlinkAddress(InetSocketAddress(userIpIfaces_2.GetAddress(i), dlPort));
        serverSender->Setup(serverSocket, downlinkAddress, dlPacketSize, 1000000, dlPacketInterval);
        remoteServer->AddApplication(serverSender);
        ftpApps.Add(serverSender);

        serverSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
        remoteServer->StartApplication(Seconds(x->GetValue()));
       }

    //Set start & stop time for sinks
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(duration));


    //Trace Flows
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    flowMonitor->Start(Seconds(0.0));
	
    std::cout << "***Simulation is Starting***\n";

    Simulator::Stop(Seconds(duration));
    Simulator::Run();

    flowMonitor->SerializeToXmlFile("lte-flow.xml", true, true);

    //Calculate Results
    double dlThroughput_1 = 0, dlThroughput_2 = 0, downloadTime_1 = 0, downloadTime_2 = 0;

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
    	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    	if(t.destinationPort == dlPort_1) {
    		dlThroughput_1 += iter->second.rxBytes * 8.0 /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstRxPacket.GetSeconds()) / 1000000;
    		downloadTime_1 += iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstRxPacket.GetSeconds();
    	}
    	else if(t.destinationPort == dlPort_2) {
    		dlThroughput_2 += iter->second.rxBytes * 8.0 /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstRxPacket.GetSeconds()) / 1000000;
    		downloadTime_2 += iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstRxPacket.GetSeconds();
    	}

    }

    dlThroughput_1 /= numOfUsers_1;
    dlThroughput_2 /= numOfUsers_2;

    downloadTime_1 /= numOfUsers_1;
    downloadTime_2 /= numOfUsers_2;

    std::cout << "***Results***\n";
    std::cout << "Average DL_1 throughput: " << dlThroughput_1 << " Mbps\n";
    std::cout << "Average DL_2 throughput: " << dlThroughput_2 << " Mbps\n";
    std::cout << "Average Download Time 1: " << downloadTime_1 << " s\n";
    std::cout << "Average Download Time 2: " << downloadTime_2 << " s\n";
    Simulator::Destroy();

    return 0;
}
