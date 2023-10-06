#include <iostream>
#include <fstream>
#include <string>

#include "ns3/point-to-point-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/quic-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <arpa/inet.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Simulation_2");

bool isClient(Ipv4Address ipAddress, Ipv4InterfaceContainer tcpAddress, Ipv4InterfaceContainer quicAddress  ) {
	for(uint32_t i = 0; i < tcpAddress.GetN(); ++i) {
		Ipv4Address clientAddress = tcpAddress.Get(i).first->GetAddress(1, 0).GetLocal();
		if(clientAddress == ipAddress) {
			return true;
		}
	}

	for(uint32_t i = 0; i < quicAddress.GetN(); ++i) {
		Ipv4Address clientAddress = quicAddress.Get(i).first->GetAddress(1, 0).GetLocal();
		if(clientAddress == ipAddress) {
			return true;
		}
	}
	return false;
}

//Define function for Throughput Calculation every second

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, Ipv4InterfaceContainer tcpIpIfaces, Ipv4InterfaceContainer quicIpIfaces, Ptr<OutputStreamWrapper> temp) {


	*temp->GetStream () << Simulator::Now ().GetSeconds ();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());	
	std::map<FlowId, FlowMonitor::FlowStats> stats = flowMon->GetFlowStats();
	
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

		if (isClient(t.destinationAddress, tcpIpIfaces, quicIpIfaces)) {

			*temp->GetStream () << "," << ((iter->second.rxBytes * 8.0) / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1000000);
		}		
	}
	
	*temp->GetStream() << std::endl;

	Simulator::Schedule(Seconds(1.0),&ThroughputMonitor, fmhelper, flowMon, tcpIpIfaces, quicIpIfaces, temp);
}


void printCurrentTime() {
	
	std::cout << Simulator::Now().GetSeconds() << "\n";
}


int main(int argc, char *argv[]) {

	// Simulation setup parameters
	uint16_t tcp_flows = 0, quic_flows = 1;
	uint16_t Cells = 1;
	double distance = -3000;
	Vector eNBposition = Vector(distance, 0, 15);
	Vector PGWposition = Vector(0, 0, 0);
	Vector serverPosition = Vector(5000, 0, 0);	

	//QUIC parameters
	double numStreams = 1;
	bool use_0RTT = false;
	bool use_2RTT = true;

	//TCP parameters
	std::string tcp_congestion_op = "TcpNewReno";
	double fileSize = 0;	
	
	//General parameters
	double error_p = 0.0;
	double duration = 20.0;
	double packetSize = 1024;
	double nPackets = 10000000; //512 KB
	double interval = 100; //microseconds
	double UE_radius = 50; //meters
	uint64_t buffSize = 524288;
	
	// Parse command line attribute
	CommandLine cmd;

	cmd.AddValue ("duration", "The duration of the simulation", duration);
	cmd.AddValue ("tcp_flows", "The number of TCP clients", tcp_flows);
	cmd.AddValue ("quic_flows", "The number of QUIC clients", quic_flows);
	cmd.AddValue ("buffSize", "The buffer size of TCP/QUIC sockets", buffSize);
	cmd.AddValue ("tcp_congestion_op", "The TCP congestion algorithm", tcp_congestion_op);
	cmd.AddValue ("error_p", "The packet error probability on PGW-server link", error_p);
	cmd.AddValue ("UE_radius", "The distance between UE and eNodeB", UE_radius);
	cmd.AddValue ("nPackets", "The total number of packets to be sent", nPackets);
	cmd.AddValue ("packetSize", "The size of packets in bytes", packetSize);
	cmd.AddValue ("fileSize", "The file size for bulksend TCP app", fileSize);
	cmd.AddValue ("use_0RTT", "Enable 0-RTT Handshake", use_0RTT);
	cmd.AddValue ("use_2RTT", "Enable 2-RTT Handshake", use_2RTT);
	cmd.AddValue ("numStreams", "Set the number of streams to be used", numStreams);
	cmd.AddValue ("interval", "The time interval between packets (in microseconds)", interval);
	

	cmd.Parse(argc, argv);

	LogComponentEnable("Simulation_2", LOG_LEVEL_ALL);

	tcp_congestion_op = std::string ("ns3::") + tcp_congestion_op;

	LogComponentEnableAll (LOG_PREFIX_TIME);
	LogComponentEnableAll (LOG_PREFIX_FUNC);
	LogComponentEnableAll (LOG_PREFIX_NODE);

	LogComponentEnable ("QuicSocketBase", LOG_LEVEL_INFO);
	LogComponentEnable ("TcpSocketBase", LOG_LEVEL_INFO);

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

	//Socket TCP/QUIC Parameters
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(buffSize));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(buffSize));
	Config::SetDefault("ns3::UdpSocket::RcvBufSize", UintegerValue(buffSize));
	Config::SetDefault ("ns3::QuicSocketBase::SocketRcvBufSize", UintegerValue (buffSize));
  	Config::SetDefault ("ns3::QuicSocketBase::SocketSndBufSize", UintegerValue (buffSize));
  	Config::SetDefault ("ns3::QuicStreamBase::StreamSndBufSize", UintegerValue (buffSize));
  	Config::SetDefault ("ns3::QuicStreamBase::StreamRcvBufSize", UintegerValue (buffSize));
	Config::SetDefault ("ns3::QuicL4Protocol::0RTT-Handshake", BooleanValue (use_0RTT));

	if (use_2RTT == true) {

		Config::SetDefault ("ns3::QuicSocketBase::InitialVersion", UintegerValue (QUIC_VERSION_NEGOTIATION));
	}
	else {

		Config::SetDefault ("ns3::QuicSocketBase::InitialVersion", UintegerValue (QUIC_VERSION_NS3_IMPL));
	}


	// Select congestion control variant
	if (tcp_congestion_op.compare ("ns3::TcpWestwoodPlus") == 0) {

		// TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
		// the default protocol type in ns3::TcpWestwood is WESTWOOD
		Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
		Config::SetDefault ("ns3::QuicL4Protocol::SocketType", TypeIdValue(QuicCongestionOps::GetTypeId()));
	}
	else {
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcp_congestion_op)));
		//Config::SetDefault ("ns3::QuicL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcp_congestion_op)));
		Config::SetDefault ("ns3::QuicL4Protocol::SocketType", TypeIdValue(QuicCongestionOps::GetTypeId()));
	}	  


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

	epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(5)));
	epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue(DataRate("1Gbps")));
	epcHelper->SetAttribute("S1uLinkMtu", UintegerValue(2000));
	epcHelper->SetAttribute("S1uLinkEnablePcap", BooleanValue(false));

	//Set RRC protocol
	lteHelper->SetAttribute("UseIdealRrc", BooleanValue(true));

	//Set Propagation Loss
	lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisPropagationLossModel"));
	lteHelper->SetPathlossModelAttribute("Frequency", DoubleValue(2145000000));
	
	//Set Wireless Fading
	lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
	lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("/home/ns3/Desktop/quic-ns-3/scratch/fading_trace_modified_10s_EPA_3kmph.fad"));
	lteHelper->SetFadingModelAttribute ("TraceLength", TimeValue (Seconds (10.0)));
	lteHelper->SetFadingModelAttribute ("SamplesNum", UintegerValue (25000));
	lteHelper->SetFadingModelAttribute ("WindowSize", TimeValue (Seconds (0.5)));
	lteHelper->SetFadingModelAttribute ("RbNum", UintegerValue (100));

	//Set Antenna Type
    lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
	lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");

	//Set Scheduler Type
    lteHelper->SetAttribute("Scheduler", StringValue("ns3::RrFfMacScheduler"));
	lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(false));
	
	//Set mobility for eNB nodes and UE nodes
	MobilityHelper eNBmobility;	
	Ptr<ListPositionAllocator> eNBposition_allocator = CreateObject<ListPositionAllocator> ();
    eNBposition_allocator->Add(eNBposition);
	eNBmobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	eNBmobility.SetPositionAllocator(eNBposition_allocator);
	eNBmobility.Install(enBNodes.Get(0));

	Ptr<ConstantRandomVariable> rho = CreateObject<ConstantRandomVariable>();
    rho->SetAttribute ("Constant", DoubleValue (UE_radius));

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
	QuicHelper stackQuic;
	stackQuic.InstallQuic(remoteServerContainer);

	//Set mobility for server node
	MobilityHelper servermobility;	
	Ptr<ListPositionAllocator> serverPosition_allocator = CreateObject<ListPositionAllocator> ();
    serverPosition_allocator->Add(serverPosition);
	servermobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	servermobility.SetPositionAllocator(serverPosition_allocator);
	servermobility.Install(remoteServer);

	//Create Error Model for Internet Link
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  	RateErrorModel error_model;
  	error_model.SetRandomVariable (uv);
  	error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  	error_model.SetRate (error_p);

	// Create the Internet link
    PointToPointHelper remoteServerp2p;//point to point line between PGW and remote server
	remoteServerp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
	remoteServerp2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(12)));
	remoteServerp2p.SetDeviceAttribute("Mtu", UintegerValue(2000));
	remoteServerp2p.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));

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

    stackQuic.InstallQuic(tcpNodes);
    stackQuic.InstallQuic(quicNodes);
    Ipv4InterfaceContainer tcpIpIfaces, quicIpIfaces;
    tcpIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(tcpLteDevs));
    quicIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(quicLteDevs));

	//Create Routing Information

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

	//Create Random Variables for Application Start Time
	double min = 1.0;
    double max = 3.0;
    Ptr<UniformRandomVariable> startTimes = CreateObject<UniformRandomVariable> ();
    startTimes->SetAttribute ("Min", DoubleValue (min));
    startTimes->SetAttribute ("Max", DoubleValue (max));

	//Create application for both TCP and QUIC clients

	uint16_t dlPort_tcp = 1000;
	uint16_t dlPort_quic = 2000;

	for(uint16_t i = 0; i < tcpNodes.GetN(); i++) {

		//Create TCP apps
		
		AddressValue remoteAddress (InetSocketAddress (tcpIpIfaces.GetAddress (i, 0), dlPort_tcp));
		BulkSendHelper tcpHelper ("ns3::TcpSocketFactory", Address());
		tcpHelper.SetAttribute("Remote", remoteAddress);
		tcpHelper.SetAttribute("MaxBytes", UintegerValue(fileSize));
		tcpHelper.SetAttribute("SendSize", UintegerValue(packetSize));
		ApplicationContainer tcpApp = tcpHelper.Install(remoteServer);

		tcpApp.Get(0)->SetStartTime(Seconds(1.0 + (i*0.1)));
		//tcpApp.Get(0)->SetStopTime(Seconds(duration-1.0));	

		//Create Packet Sink on UE
		Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny(), dlPort_tcp));
		PacketSinkHelper dlPacketSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
		dlPacketSinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
		ApplicationContainer sinkApp = dlPacketSinkHelper.Install(tcpNodes.Get(i));

		sinkApp.Get(0)->SetStartTime(Seconds(0.5));
		sinkApp.Get(0)->SetStopTime(Seconds(duration));
	}



	for(uint16_t i = 0; i < quicNodes.GetN(); i++) {

		//Create QUIC client - server app

		QuicServerHelper dlPacketSinkHelper (dlPort_quic);
     	AddressValue remoteAddress (InetSocketAddress (quicIpIfaces.GetAddress (i, 0), dlPort_quic));
     	ApplicationContainer sinkApp = dlPacketSinkHelper.Install(quicNodes.Get(i));

     	QuicClientHelper dlClient (quicIpIfaces.GetAddress (i, 0), dlPort_quic);
     	dlClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interval)));
    	dlClient.SetAttribute ("PacketSize", UintegerValue(packetSize));
     	dlClient.SetAttribute ("MaxPackets", UintegerValue(nPackets));
     	//dlClient.SetAttribute ("NumStreams", UintegerValue(numStreams));
     	ApplicationContainer quicApp = dlClient.Install (remoteServer);

     	quicApp.Get(0)->SetStartTime(Seconds(2.0 + (i*0.1)));
		quicApp.Get(0)->SetStopTime(Seconds(duration-1));

		/*//Create Packet Sink on UE
		Address sinkAddressAny (InetSocketAddress (Ipv4Address::GetAny (), dlPort_quic));
		PacketSinkHelper dlPacketSinkHelper("ns3::QuicSocketFactory", sinkAddressAny);
		dlPacketSinkHelper.SetAttribute ("Protocol", TypeIdValue (QuicSocketFactory::GetTypeId ()));
		ApplicationContainer sinkApp = dlPacketSinkHelper.Install(quicNodes.Get(i));*/

		sinkApp.Get(0)->SetStartTime(Seconds(0.5));
		sinkApp.Get(0)->SetStopTime(Seconds(duration));		
	}

	//Schedule Time Print

	for(int i = 1; i < duration; ++i) {
    	Simulator::Schedule(Seconds(i), &printCurrentTime);
    }

	//Schedule Traces

	/*for (uint16_t i = 0; i < tcp_flows; i++)
    {
 		//Make paths, files and schedule traces

		Simulator::Schedule (Seconds(1.1), &Traces, remoteServer->GetId());
    }

	for (uint16_t i = 0; i < quic_flows; i++)
    {
      Ptr<Node> quicClient = quicNodes.Get (i);
      Ptr<Node> server = remoteServerContainer.Get (0);
      Time t = Seconds(1.1);
      Simulator::Schedule (t, &Traces, server->GetId(), "Quic", ".txt", i);
    }*/
	

	//Configure Flow Monitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    flowMonitor->Start(Seconds(0.0));

	//Create files for Throughput Calculations
	AsciiTraceHelper asciiHelper;

	std::ostringstream fileThroughput;

  	fileThroughput << "Throughput_Calculations.csv";

	Ptr<OutputStreamWrapper> streamThroughput = asciiHelper.CreateFileStream (fileThroughput.str ().c_str ());

	//Schedule Flow Monitor every second
	//Simulator::Schedule(Seconds(0.6), &ThroughputMonitor, &flowHelper, flowMonitor, tcpIpIfaces, quicIpIfaces, streamThroughput);


	//Enable Pcap tracing for server
	//remoteServerp2p.EnablePcap("trace", remoteServerContainer, BooleanValue(false));
	
    std::cout << "***Simulation is Starting***" << std::endl;

    Simulator::Stop(Seconds(duration));
    Simulator::Run();

    flowMonitor->SerializeToXmlFile("simulation-flow.xml", true, true);

    //Calculate Final Results
    float dlThroughput_tcp = 0, dlThroughput_quic = 0;
	float downloadTime_tcp = 0, downloadTime_quic = 0;

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
    	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    	if(t.destinationPort == dlPort_tcp) {
    		//dlThroughput_tcp += (nPackets * packetSize * 8.0)/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000000;
    		dlThroughput_tcp += (iter->second.rxBytes * 8.0) / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000000;
    		downloadTime_tcp += iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds();
    	}

		
    	if(t.destinationPort == dlPort_quic) {
    		//dlThroughput_quic += (nPackets * packetSize * 8.0)/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000000;
    		dlThroughput_quic += (iter->second.rxBytes * 8.0) /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000000;
    		downloadTime_quic += iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds();
    	}     	
    }

    //dlThroughput_tcp /= tcp_flows;
	dlThroughput_quic /= quic_flows;

    //downloadTime_tcp /= tcp_flows;
	downloadTime_quic /= quic_flows;

    std::cout << "***Results***\n";
    std::cout << "Average DL throughput for TCP clients: " << dlThroughput_tcp << " Mbps\n";
	std::cout << "Average DL throughput for QUIC clients: " << dlThroughput_quic << " Mbps\n";
    std::cout << "Average Download Time for TCP clients : " << downloadTime_tcp << " s\n";
	std::cout << "Average Download Time for QUIC clients : " << downloadTime_quic << " s\n";
    Simulator::Destroy();


    return 0;


	
}
