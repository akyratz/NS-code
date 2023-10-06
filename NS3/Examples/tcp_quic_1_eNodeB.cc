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

void printCurrentTime() {
	std::cout << Simulator::Now().GetSeconds() << "\n";
}


int main(int argc, char **argv) {

    //Set LTE physical layer parameters

	//Bandwith and Frequencies
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue(2300));
	Config::SetDefault("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(20300));
	Config::SetDefault("ns3::LteUeNetDevice::DlEarfcn", UintegerValue(2300));

	//Transmission Power and Noise
	Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(46));
	Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5));
	Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(23));
	Config::SetDefault("ns3::LteUePhy::NoiseFigure", DoubleValue(9));
	Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));

	//LTE RRC/RLC parameters
	Config::SetDefault ("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue (2));
	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(40));
	Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", StringValue("RlcAmAlways"));
	Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(524288));

	//LTE AMC Model
	Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
	Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005)); 

	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500));


	uint16_t tcp_flows = 1, quic_flows = 0;
	uint16_t Cells = 1;
	double distance = -3000;
	Vector eNBposition = Vector(distance, 0, 15);
	Vector PGWposition = Vector(0, 0, 0);

	double duration = 60;

	// Parse command line attribute
	CommandLine cmd;
	//add attributes

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();
	//parse again so you can override default values from the command line
	cmd.Parse(argc, argv);


	//Create UE and eNB nodes
	NodeContainer tcpNodes, quicNodes, enBNodes;
	tcpNodes.Create(tcp_flows);
	quicNodes.Create(quic_flows);
	enBNodes.Create(Cells);

	//Create LTE and EPC
	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
	Ptr<Node> pgwNode = epcHelper->GetPgwNode(); //P-GW node

	epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(14)));
	epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue(DataRate("1Gb/s")));
	epcHelper->SetAttribute("S1uLinkMtu", UintegerValue(2000));
	epcHelper->SetAttribute("S1uLinkEnablePcap", BooleanValue(false));

	//Set RRC protocol
	lteHelper->SetAttribute("UseIdealRrc", BooleanValue(false));

	//Set Propagation Loss
	lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));
	lteHelper->SetPathlossModelAttribute("Frequency", DoubleValue(2145000000));
	
	//Set Wireless Fading
	lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
	lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("/home/ns3/Desktop/quic-ns-3/scratch/fading_trace_ETU_3kmph.fad"));

	//Set Antenna Type
    lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
	lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");

	//Set Scheduler Type
    lteHelper->SetAttribute("Scheduler", StringValue("ns3::RrFfMacScheduler"));
	
	//Set mobility for eNB nodes and UE nodes
	MobilityHelper eNBmobility;	
	Ptr<ListPositionAllocator> eNBposition_allocator = CreateObject<ListPositionAllocator> ();
    eNBposition_allocator->Add(eNBposition);
	eNBmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	eNBmobility.SetPositionAllocator(eNBposition_allocator);
	eNBmobility.Install(enBNodes.Get(0));

	Ptr<UniformRandomVariable> rho = CreateObject<UniformRandomVariable>();
    rho->SetAttribute ("Min", DoubleValue (200));
    rho->SetAttribute ("Max", DoubleValue (500));

	Ptr<UniformRandomVariable> theta = CreateObject<UniformRandomVariable>();
    theta->SetAttribute ("Min", DoubleValue (0));
    theta->SetAttribute ("Max", DoubleValue (6.2830));

	MobilityHelper UEmobility;	
	Ptr<RandomDiscPositionAllocator> UEposition_allocator = CreateObject<RandomDiscPositionAllocator> ();
	UEposition_allocator->SetRho(rho);
	UEposition_allocator->SetTheta(theta);
	UEposition_allocator->SetX(eNBposition.x);
	UEposition_allocator->SetY(eNBposition.y);
	UEposition_allocator->SetZ(eNBposition.z);
	UEmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	UEmobility.SetPositionAllocator(UEposition_allocator);
	UEmobility.Install(tcpNodes);
	UEmobility.Install(quicNodes);

	MobilityHelper PGWmobility;	
	Ptr<ListPositionAllocator> PGWposition_allocator = CreateObject<ListPositionAllocator> ();
    PGWposition_allocator->Add(PGWposition);
	PGWmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	PGWmobility.SetPositionAllocator(PGWposition_allocator);
	PGWmobility.Install(pgwNode);

	//Install LTE stack to UE and eNB nodes
    NetDeviceContainer eNBLteDevs = lteHelper->InstallEnbDevice(enBNodes);
    NetDeviceContainer tcpLteDevs, quicLteDevs;
    tcpLteDevs = lteHelper->InstallUeDevice(tcpNodes);
    quicLteDevs = lteHelper->InstallUeDevice(quicNodes);

	//Create remote server and add IP stack
	NodeContainer remoteServerContainer;
	remoteServerContainer.Create(1);
	Ptr<Node> remoteServer = remoteServerContainer.Get(0);
	InternetStackHelper internet;
	internet.Install(remoteServerContainer);

	// Create the Internet link
    PointToPointHelper remoteServerp2p;//point to point line between PGW and remote server
	remoteServerp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gb/s")));
	remoteServerp2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(18)));
	remoteServerp2p.SetDeviceAttribute("Mtu", UintegerValue(2000));

    NetDeviceContainer internetDevices;
    internetDevices = remoteServerp2p.Install(pgwNode, remoteServer);
    Ipv4AddressHelper ipv4helper;
    ipv4helper.SetBase("1.0.0.0", "255.255.255.252");
    Ipv4InterfaceContainer internetIpIfaces = ipv4helper.Assign(internetDevices);
    Ipv4Address remoteServerAddr = internetIpIfaces.GetAddress (1);
    Ipv4Address pgwServerAddr = internetIpIfaces.GetAddress(0);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteServerStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteServer->GetObject<Ipv4> ());
    remoteServerStaticRouting->SetDefaultRoute(pgwServerAddr, 1);

	// Install IP stack on UE
    internet.Install(tcpNodes);
    internet.Install(quicNodes);
    Ipv4InterfaceContainer tcpIpIfaces, quicIpIfaces;
    tcpIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(tcpLteDevs));
    quicIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(quicLteDevs));

    for(uint32_t i = 0; i < tcpNodes.GetN(); ++i) {
    	Ptr<Node> userNode = tcpNodes.Get(i);
        // Set the default gateway for the tcp UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(userNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    for(uint32_t i = 0; i < quicNodes.GetN(); ++i) {
      	Ptr<Node> userNode = quicNodes.Get(i);
        // Set the default gateway for the quic UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(userNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    //Attach UE's to eNodeB
    lteHelper->Attach (tcpLteDevs, eNBLteDevs.Get (0));
    lteHelper->Attach (quicLteDevs, eNBLteDevs.Get (0));

	//Create UDP application to measure available bandiwth, by saturating DL link
	uint16_t dlPort = 9;

	for(uint32_t i = 0; i < tcpNodes.GetN(); i++) {

		PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
		ApplicationContainer sinkApp = dlPacketSinkHelper.Install(tcpNodes.Get(i));

		sinkApp.Get(0)->SetStartTime(Seconds(0.0));
		sinkApp.Get(0)->SetStopTime(Seconds(duration));

		Ptr<ConstantRandomVariable> onRandom = CreateObject<ConstantRandomVariable>();
		onRandom->SetAttribute("Constant", DoubleValue(60));

		Ptr<ConstantRandomVariable> offRandom = CreateObject<ConstantRandomVariable>();
		offRandom->SetAttribute("Constant", DoubleValue(1));

		Address dlAddress(InetSocketAddress(tcpIpIfaces.GetAddress(i), dlPort));
		OnOffHelper dlApplication("ns3::TcpSocketFactory", dlAddress);
		//dlApplication.SetAttribute("DataRate", DataRateValue(DataRate("500Mbps")));
		//dlApplication.SetAttribute("PacketSize", UintegerValue(1000));
		//dlApplication.SetAttribute("onTime",PointerValue(onRandom));
		//dlApplication.SetAttribute("offTime", PointerValue(offRandom));
		dlApplication.SetConstantRate(DataRate("50Mbps"), 1000);

		ApplicationContainer udpApp = dlApplication.Install(remoteServer);

		udpApp.Get(0)->SetStartTime(Seconds(1.0));
		udpApp.Get(0)->SetStopTime(Seconds(duration-1));
	}

	for(int i = 1; i < duration; ++i) {
    	Simulator::Schedule(Seconds(i), &printCurrentTime);
    }

	//Configure Flow Monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    flowMonitor->Start(Seconds(0.0));

	remoteServerp2p.EnablePcap("lte", remoteServerContainer, BooleanValue(false));
	
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

    	if(t.destinationPort == dlPort) {
    		dlThroughput_1 += iter->second.rxBytes * 8.0 /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000000;
    		downloadTime_1 += iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds();
    	}    	
    }

    dlThroughput_1 /= tcp_flows;
    //dlThroughput_2 /= numOfUsers_2;

    downloadTime_1 /= tcp_flows;
    //downloadTime_2 /= numOfUsers_2;

    std::cout << "***Results***\n";
    std::cout << "Average DL_1 throughput: " << dlThroughput_1 << " Mbps\n";
    //std::cout << "Average DL_2 throughput: " << dlThroughput_2 << " Mbps\n";
    std::cout << "Average Download Time 1: " << downloadTime_1 << " s\n";
    //std::cout << "Average Download Time 2: " << downloadTime_2 << " s\n";
    Simulator::Destroy();

    return 0;


}