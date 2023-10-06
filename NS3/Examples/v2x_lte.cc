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

Ipv4Address getNetworkAddress(Ipv4Address ipAddress) {
	return ipAddress.CombineMask(Ipv4Mask("255.255.255.0"));
}

std::string IpToString(uint32_t ip) {
	return inet_ntoa(*(struct in_addr *)&ip);
}

bool isVehicle(Ipv4Address ipAddress, Ipv4InterfaceContainer vehicleIpIfaces) {
	for(uint32_t i = 0; i < vehicleIpIfaces.GetN(); ++i) {
		Ipv4Address vehicleAddress = vehicleIpIfaces.Get(i).first->GetAddress(1, 0).GetLocal();
		if(vehicleAddress == ipAddress) {
			return true;
		}
	}
	return false;
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
		:	m_socket(0),
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

		void packetReceivedFromVehicle(Ptr<const Packet> packet, const Address &address) {
			//Start downlink application when packet is received from vehicle
			if(!m_running) {
				Start();
			}
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

class EmergencyDlApp : public Application {
	private:
		Ptr<Socket> m_socket;
	    Address m_peer;
		EventId m_sendEvent;
		bool m_running;
	    uint32_t m_packetsSent;

	public:
		EmergencyDlApp()
		:	m_socket(0),
			m_peer(),
			m_sendEvent(),
			m_running(false),
			m_packetsSent(0)
		{
		}

		virtual ~EmergencyDlApp() {
			m_socket = 0;
		}

		void Setup(Ptr<Socket> socket, Address address) {
			m_socket = socket;
			m_peer = address;
		}

	    void Start() {
	    	m_running = true;
	    	m_packetsSent = 0;
	    	m_socket->Bind();
	    	m_socket->Connect(m_peer);
	    }

	    void SendPacket(int packetSize) {
	    	Ptr<Packet> packet = Create<Packet>(packetSize);
	    	m_socket->Send(packet);
	    	m_packetsSent++;
	    }

	    void Stop() {
	    	m_running = false;
	    	if(m_sendEvent.IsRunning()) {
	    		Simulator::Cancel(m_sendEvent);
	    	}
	    	if(m_socket) {
	    		m_socket->Close();
	    	}
	    	m_packetsSent = 0;
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

void deAttach(Ptr<NetDevice> ue, NetDeviceContainer enbDevs) {
//	Ptr<LteUeNetDevice> UeDevice = DynamicCast<LteUeNetDevice>(*i);

	Ptr<LteUeNetDevice> ueDevice = ue->GetObject<LteUeNetDevice>();

	uint16_t cellId = ueDevice->GetRrc()->GetCellId();//turn off from the network

	uint16_t counter=1;
	Ptr<LteEnbNetDevice> enbDevice;
	for (NetDeviceContainer::Iterator enb = enbDevs.Begin(); enb != enbDevs.End (); ++enb) {
		if (counter == cellId) {
//			eNb= DynamicCast<LteEnbNetDevice> (*i2);
			enbDevice = (*enb)->GetObject<LteEnbNetDevice>();
			break;
		}
		counter++;
	}
	uint16_t rnti = ueDevice->GetRrc()->GetRnti();
//	uint64_t imsi = ueDevice->GetObject<LteUeNetDevice>()->GetImsi ();

	Ptr<LteEnbRrc> enbRrc = enbDevice->GetRrc();
	Ptr<LteEnbMac> enbMac = enbDevice->GetMac();
	//Ptr<UeManager> ueMag = enbRrc-> GetUeManager (rnti);
	//enum sta = ueMag->GetState ();
	enbMac->GetLteEnbCmacSapProvider()->ReleaseLc(rnti, 1);
	Ptr<LteUeRrc> ueRrc = ueDevice->GetRrc();

//	enbRrc->RemoveUe(rnti);
	enbRrc->ConnectionRequestTimeout(rnti);//calls removeUe
	ueRrc->GetAsSapProvider()->Disconnect();
//	enbRrc->DoSendReleaseDataRadioBearer(imsi, rnti, 0);
}

//eNB-vehicle attachment class
class Attachment {
	private:
		Ptr<LteHelper> lteHelper;
		Ptr<Node> enbNode, vehicleNode;
		NetDeviceContainer enbLteDevs;
		Ptr<MyApp> vehicleSender, serverSender;
		bool isAttached, isRunning;

	public:
		Attachment(Ptr<LteHelper> lteHelper, Ptr<MyApp> vehicleSender, Ptr<MyApp> serverSender, Ptr<Node> enbNode, Ptr<Node> vehicleNode, NetDeviceContainer enbLteDevs) {
			this->lteHelper = lteHelper;
			this->vehicleSender = vehicleSender;
			this->serverSender = serverSender;
			this->enbNode = enbNode;
			this->vehicleNode = vehicleNode;
			this->enbLteDevs = enbLteDevs;
			isAttached = false;
			isRunning = false;
		}

		//gets called each time a vehicle changes position
		void CourseChange(Ptr<const MobilityModel> model) {
			double enbRange = 1250;
			Vector enbPosition = enbNode->GetObject<MobilityModel>()->GetPosition();
			Vector position = model->GetPosition();//vehicle position
			double distance = sqrt(pow(position.x-enbPosition.x,2) + pow(position.y-enbPosition.y,2));//distance between eNB and vehicle
			if(distance < enbRange && !isAttached) {//vehicle is within eNB range and it is not attached
				lteHelper->Attach(vehicleNode->GetDevice(0), enbNode->GetDevice(0));//attach vehicle to eNB
//				//Activate Eps bearer
//				Ptr<EpcTft> tft = Create<EpcTft>();
//				GbrQosInformation qos;
//				EpsBearer bearer(EpsBearer::NGBR_VOICE_VIDEO_GAMING, qos);
//				lteHelper->ActivateDedicatedEpsBearer(vehicleNode->GetDevice(0), bearer, tft);

				//instantiate applications
				int timeOffset = rand() % 100;//random time offset for each vehicle
				vehicleSender->StartApplication(MilliSeconds(timeOffset));
				isAttached = true;
				isRunning = true;
			} else if(distance > enbRange && isRunning) {
				//terminate applications
				vehicleSender->StopApplication(Seconds(0));
				serverSender->StopApplication(Seconds(0));
//				lteHelper->DeActivateDedicatedEpsBearer(vehicleNode->GetDevice(0), enbNode->GetDevice(0), 2);
				isRunning = false;
			} else if(distance > enbRange + 600 && isAttached) {
				isAttached = false;
				deAttach(vehicleNode->GetDevice(0), enbLteDevs);
			}
		}
};

//finds the event number that corresponds to a given start time
int findEmergencyNumber(Time startTime, int emergencyTimes[], int numOfEmergencies) {

	int startSeconds = startTime.GetSeconds();
	for(int i = 0; i < numOfEmergencies - 1; ++i) {
		if(startSeconds < emergencyTimes[i+1]) {
			return i;
		}
	}
	return numOfEmergencies - 1;
}

//emergency server sender application class
class Emergency {
	private:
		NodeContainer vehicleNodes;
		Ptr<Node> v2xServer;
		uint16_t dlPort;
		ApplicationContainer serverSenderApps;

		Ptr<Node> findVehicleFromIp(Ipv4Address vehicleAddress) {
			for(uint32_t i = 0; i < vehicleNodes.GetN(); ++i) {
			    Ptr<Node> vehicleNode = vehicleNodes.Get(i);
			    Ptr<Ipv4> ipv4 = vehicleNode->GetObject<Ipv4>();
			    Ipv4Address address = ipv4->GetAddress(1, 0).GetLocal();
			    if(address.Get() == vehicleAddress.Get()) {
			    	return vehicleNode;
			    }
			}
			return NULL;//vehicle was not found
		}

	public:
		Emergency(NodeContainer vehicleNodes, Ptr<Node> v2xServer, uint16_t dlPort, ApplicationContainer serverSenderApps) {
			this->vehicleNodes = vehicleNodes;
			this->v2xServer = v2xServer;
			this->dlPort = dlPort;
			this->serverSenderApps = serverSenderApps;
		}
		//gets called when server receives an emergency packet
		void emergencyPacketReceived(Ptr<const Packet> packet, const Address &address) {
//			Ptr<Node> sender = vehicleNodes.Get(emergencyVehicleIndex);//vehicle that sent the message
			InetSocketAddress senderAddress = InetSocketAddress::ConvertFrom(address);
			Ipv4Address senderIpAddress = senderAddress.GetIpv4();
			Ptr<Node> sender = findVehicleFromIp(senderIpAddress);//vehicle that send the message
			Ptr<MobilityModel> mobility = sender->GetObject<MobilityModel>();
			Vector senderPosition = mobility->GetPosition(); //position of sender vehicle
			int maxDistance = 500;
			for(uint16_t i = 0; i < vehicleNodes.GetN(); ++i) {
				Ptr<Node> vehicle = vehicleNodes.Get(i);
				mobility = vehicle->GetObject<MobilityModel>();
				Vector vehiclePosition = mobility->GetPosition();
				double distance = sqrt(pow(vehiclePosition.x-senderPosition.x,2) + pow(vehiclePosition.y-senderPosition.y,2));//distance between sender vehicle and other vehicle
				if(distance < maxDistance && vehicle->GetId() != sender->GetId()) {//vehicle is within range but not the sender
					Ptr<Application> app = serverSenderApps.Get(i);
					Ptr<MyApp> serverSender = app->GetObject<MyApp>();//downcast to MyApp
					if(serverSender->IsRunning()) {//server application for this vehicle has been initialized
						//create socket to send emergency packets to each vehicle
						Ptr<Packet> copy = packet->Copy();//copy packet to send
						//remove ip header
						Ipv4Header iph;
						copy->RemoveHeader(iph);
						int packetSize = copy->GetSize();
						//connect socket and send packet
//						Ptr<Packet> copy = Create<Packet>(300);

			        	//Create emergency dl application for each vehicle at server
			        	Ptr<Socket> emergencyDlSocket = Socket::CreateSocket(v2xServer, UdpSocketFactory::GetTypeId());
			        	Ptr<EmergencyDlApp> emergencyDlSender = CreateObject<EmergencyDlApp>();

			        	Ipv4Address vehicleAddr = vehicle->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
			        	Address emergencyDlAddress(InetSocketAddress(vehicleAddr, dlPort));
			        	emergencyDlSender->Setup(emergencyDlSocket, emergencyDlAddress);
			        	v2xServer->AddApplication(emergencyDlSender);
			        	emergencyDlSender->Start();
						Ptr<EmergencyDlApp> emergencyDlApp = emergencyDlSender->GetObject<EmergencyDlApp>();
						emergencyDlApp->SendPacket(packetSize);
					}
				}
			}
		}
};

void sendEmergencyMessage(NodeContainer vehicleNodes, Ptr<Node> sender, Ipv4Address v2xServerAddr, uint16_t ulPort) {
	//packet size = 190, 300, 800
	int packetSize = 800;

	Ptr<Socket> emergencyVehicleSocket = Socket::CreateSocket(sender, UdpSocketFactory::GetTypeId());
	Address uplinkAddress(InetSocketAddress(v2xServerAddr, ulPort));
	emergencyVehicleSocket->Bind();
	emergencyVehicleSocket->Connect(uplinkAddress);
	Ptr<Packet> packet = Create<Packet>(packetSize);
	emergencyVehicleSocket->Send(packet);
}

void printEmergencyData(NodeContainer vehicleNodes, int index, NodeContainer enbNodes) {
	Ptr<Node> sender = vehicleNodes.Get(index);
	Ptr<Node> enb = enbNodes.Get(0);
	Vector senderPosition = sender->GetObject<MobilityModel>()->GetPosition();
	Vector enbPosition = enb->GetObject<MobilityModel>()->GetPosition();
	std::cout << "Vehicle " << index << " sent emergency message at " << Simulator::Now().GetSeconds() << "sec\n";
	double distanceFromEnb = sqrt(pow(enbPosition.x-senderPosition.x,2) + pow(enbPosition.y-senderPosition.y,2));
	std::cout<< "Distance between sender and enb is " << distanceFromEnb << "m\n";
	int numOfRec = 0;
	for(uint32_t i = 0; i < vehicleNodes.GetN(); ++i) {
		Vector vehiclePosition = vehicleNodes.Get(i)->GetObject<MobilityModel>()->GetPosition();
		double vehicleDistance = sqrt(pow(vehiclePosition.x-senderPosition.x,2) + pow(vehiclePosition.y-senderPosition.y,2));
		if(vehicleDistance < 500) {
			numOfRec++;
		}
	}
	std::cout << "Number of vehicles that will receive the message: " << numOfRec << "\n\n";
}

void printCurrentTime() {
	std::cout << Simulator::Now().GetSeconds() << "\n";
}

int main(int argc, char **argv) {

	Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(18100));
	Config::SetDefault("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue(100));
	Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(26));
	Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(false));
	Config::SetDefault("ns3::LteUePhy::NoiseFigure", DoubleValue(9));
//	Config::SetDefault("ns3::LteUePhy::RsrpSinrSamplePeriod", UintegerValue(100));
//	Config::SetDefault("ns3::LteEnbPhy::UeSinrSamplePeriod", UintegerValue(100));
//	Config::SetDefault("ns3::LteEnbPhy::InterferenceSamplePeriod", UintegerValue(100));
	Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(46));
	Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5));
	Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(1280));
//	Config::SetDefault("ns3::FfMacScheduler::UlCqiFilter", EnumValue(FfMacScheduler::SRS_UL_CQI));

	std::string traceFile;
	uint16_t vehicleNum, pedestrianNum;
    Vector enbPosition = Vector(6557, 7324, 30);
	double duration;
	int ulPacketInterval = 100, dlPacketInterval = 1000; //ms
	int ulPacketSize = 300, dlPacketSize = 1000; //bytes

	// Parse command line attribute
	CommandLine cmd;
	cmd.AddValue("traceFile", "Ns2 movement trace file", traceFile);
	cmd.AddValue("vehicleNum", "Number of vehicles", vehicleNum);
	cmd.AddValue("pedestrianNum", "Number of pedestrians", pedestrianNum);
	cmd.AddValue("duration", "Duration of Simulation(sec)", duration);
	cmd.AddValue("ulPacketSize", "Size of uplink packets(bytes)", ulPacketSize);
	cmd.AddValue("dlPacketSize", "Size of downlink packets(bytes)", dlPacketSize);
	cmd.AddValue("ulInterval", "Uplink interval(ms)", ulPacketInterval);
	cmd.AddValue("dlInterval", "Downlink interval(ms)", dlPacketInterval);
	cmd.Parse(argc,argv);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();
	//parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	//Create Ns2MobilityHelper for importing ns-2 format mobility trace
	Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);

	//Create vehicle nodes
	NodeContainer vehicleNodes;
	vehicleNodes.Create(vehicleNum);
	//configure movements for each node, while reading trace file
	ns2.Install();

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

	lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
	lteHelper->SetFadingModelAttribute("TraceFilename", StringValue("fading_trace_EVA_60kmph.fad"));

	lteHelper->SetAttribute("Scheduler", StringValue("ns3::RrFfMacScheduler"));
//	lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(false));

	//Create remote hosts
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create(2);
	Ptr<Node> remoteV2XServer = remoteHostContainer.Get(0);
	Ptr<Node> remoteServer = remoteHostContainer.Get(1);
	InternetStackHelper internet;
	internet.Install(remoteHostContainer); //install internet stack on remote hosts

	// Create the Internet
	PointToPointHelper remoteV2Xp2p;//point to point line between router and remote v2x server
	remoteV2Xp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Mb/s")));
	remoteV2Xp2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
	remoteV2Xp2p.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

    PointToPointHelper remoteServerp2p;//point to point line between router and remote server
	remoteServerp2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Mb/s")));
	remoteServerp2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
	remoteServerp2p.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

//	TrafficControlHelper trafficHelper;
//	trafficHelper.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", StringValue("0p"));

    NetDeviceContainer internetDevices;
    internetDevices = remoteServerp2p.Install(remoteServer, pgw);
//    trafficHelper.Install(internetDevices);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.1.0.0", "255.255.255.252");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteServerAddr = internetIpIfaces.GetAddress(0);
    Ipv4Address pgwServerAddr = internetIpIfaces.GetAddress(1);

    internetDevices = remoteV2Xp2p.Install(remoteV2XServer, pgw);
//    trafficHelper.Install(internetDevices);
    ipv4h.SetBase("1.2.0.0", "255.255.255.252");
    internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteV2XAddr = internetIpIfaces.GetAddress(0);
    Ipv4Address pgwV2XAddr = internetIpIfaces.GetAddress(1);

    //Create enb nodes and set mobility model(constant position)
    NodeContainer enbNodes;
    enbNodes.Create(1);
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(enbPosition);
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer vehicleLteDevs = lteHelper->InstallUeDevice(vehicleNodes);

//    Ptr<LteEnbNetDevice> enbLte = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>();
//    Ptr<LteEnbPhy> enbPhy = enbLte->GetPhy();
//    enbPhy->SetAttribute("TxPower", DoubleValue(46));

    // Install the IP stack on the UEs
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    internet.Install(vehicleNodes);
    Ipv4InterfaceContainer vehicleIpIfaces;
    vehicleIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(vehicleLteDevs));

    //Set default gateway of servers and vehicles
    Ptr<Ipv4StaticRouting> v2xServerStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteV2XServer->GetObject<Ipv4>());
    v2xServerStaticRouting->SetDefaultRoute(pgwV2XAddr, 1);

    Ptr<Ipv4StaticRouting> serverStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteServer->GetObject<Ipv4>());
    serverStaticRouting->SetDefaultRoute(pgwServerAddr, 1);

//    Ptr<Ipv4StaticRouting> pgwStaticRouting = ipv4RoutingHelper.GetStaticRouting(pgw->GetObject<Ipv4>());
//    pgwStaticRouting->AddHostRouteTo(remoteServerAddr, routerPGWAddr, 2);
//    pgwStaticRouting->AddHostRouteTo(remoteV2XAddr, routerPGWAddr, 2);

    for(uint32_t i = 0; i < vehicleNodes.GetN(); ++i) {
    	Ptr<Node> vehicleNode = vehicleNodes.Get(i);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(vehicleNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    //Create servers and clients
    ApplicationContainer serverSenderApps;
    uint16_t dlPort = 1000;//downlink port same for all vehicles
    uint16_t ulPort = 2000;//uplink port different for each vehicle
    uint16_t emergencyDlPort = 10000;
    uint16_t emergencyUlPort = 20000;
    for(uint16_t i = 0; i < vehicleNodes.GetN(); i++) {
    	ulPort++;
    	//Packet sink(receiver) for vehicle and server
    	PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
    	PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
    	dlPacketSinkHelper.Install(vehicleNodes.Get(i)); //Add downlink sink to each vehicle
    	ApplicationContainer ulSinkApps = ulPacketSinkHelper.Install(remoteV2XServer);

    	//Uplink application (vehicle sender)
    	Ptr<Socket> vehicleSocket = Socket::CreateSocket(vehicleNodes.Get(i), UdpSocketFactory::GetTypeId());
        Ptr<MyApp> vehicleSender = CreateObject<MyApp>();
        Address uplinkAddress(InetSocketAddress(remoteV2XAddr, ulPort));
        vehicleSender->Setup(vehicleSocket, uplinkAddress, ulPacketSize, 1000000, ulPacketInterval);
        vehicleNodes.Get(i)->AddApplication(vehicleSender);

        //Downlink application (server sender)
        Ptr<Socket> serverSocket = Socket::CreateSocket(remoteV2XServer, UdpSocketFactory::GetTypeId());
        Ptr<MyApp> serverSender = CreateObject<MyApp>();
        Address downlinkAddress(InetSocketAddress(vehicleIpIfaces.GetAddress(i), dlPort));
        serverSender->Setup(serverSocket, downlinkAddress, dlPacketSize, 1000000, dlPacketInterval);
        remoteV2XServer->AddApplication(serverSender);
        serverSenderApps.Add(serverSender);

        ulSinkApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&MyApp::packetReceivedFromVehicle, serverSender));

    	//Set callback for course change
    	std::ostringstream oss;
    	oss << "/NodeList/" << vehicleNodes.Get(i)->GetId() << "/$ns3::MobilityModel/CourseChange";
    	//eNB-vehicle attachment
    	Attachment* attachment = new Attachment(lteHelper, vehicleSender, serverSender, enbNodes.Get(0), vehicleNodes.Get(i), enbLteDevs);

    	Config::ConnectWithoutContext(oss.str(), MakeCallback(&Attachment::CourseChange, attachment));

        //Create emergency message sinks at vehicles
        PacketSinkHelper emergencyDlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), emergencyDlPort));
        emergencyDlPacketSinkHelper.Install(vehicleNodes.Get(i)); //Add downlink sink to each vehicle

    }

    //Create emergency sink at server
	PacketSinkHelper emergencyUlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), emergencyUlPort));
	ApplicationContainer emergencyUlPacketSink = emergencyUlPacketSinkHelper.Install(remoteV2XServer);

	//Create emergency message sender at server
	Emergency* emergency = new Emergency(vehicleNodes, remoteV2XServer, emergencyDlPort, serverSenderApps);
	emergencyUlPacketSink.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&Emergency::emergencyPacketReceived, emergency));

	//low mobility
	int numOfEmergencies = 11;
	int emergencyIndexes[numOfEmergencies] = {51, 179, 69, 173, 140, 120, 138, 197, 268, 201, 441}; //vehicles that send emergency messages
	int emergencyTimes[numOfEmergencies] = {550, 608, 640, 709, 758, 805, 867, 903, 984, 1054, 1100}; //time stamps of emergency messages

//	//low mobility
//	int numOfEmergencies = 2;
//	int emergencyIndexes[numOfEmergencies] = {51, 51}; //vehicles that send emergency messages
//	int emergencyTimes[numOfEmergencies] = {550, 555}; //time stamps of emergency messages

//	//medium mobility
//	int numOfEmergencies = 11;
//	int emergencyIndexes[numOfEmergencies] = {488, 691, 175, 241, 434, 682, 557, 354, 586, 699, 1139}; //vehicles that send emergency messages
//	int emergencyTimes[numOfEmergencies] = {550, 608, 640, 709, 758, 805, 867, 903, 984, 1054, 1100}; //time stamps of emergency messages


//	//high mobility
//	int numOfEmergencies = 11;
//	int emergencyIndexes[numOfEmergencies] = {0, 357, 18, 610, 95, 126, 1674, 1336, 1792, 1404, 1016}; //vehicles that send emergency messages
//	int emergencyTimes[numOfEmergencies] = {550, 608, 640, 709, 758, 805, 867, 903, 984, 1054, 1100}; //time stamps of emergency messages

	//no events
//	int numOfEmergencies = 0;
//	int emergencyIndexes[1] = {0};
//	int emergencyTimes[1] = {0};

	for(int i = 0; i < numOfEmergencies; ++i) {
		//Schedule emergency message
		Simulator::Schedule(Seconds(emergencyTimes[i]), &sendEmergencyMessage, vehicleNodes, vehicleNodes.Get(emergencyIndexes[i]), remoteV2XAddr, emergencyUlPort);
//		Simulator::Schedule(Seconds(emergencyTimes[i]), &printEmergencyData, vehicleNodes, emergencyIndexes[i], enbNodes);
	}

    //Create pedestrians
	NodeContainer pedestrianNodes;
	pedestrianNodes.Create(pedestrianNum);
	MobilityHelper pedestrianMobility;
	double x = enbPosition.x + 100, ymin = enbPosition.y - 1000, width = 1, deltaX = 0, deltaY = pedestrianNum != 0 ? 1000.0 / pedestrianNum : 0;

	pedestrianMobility.SetPositionAllocator("ns3::GridPositionAllocator", "GridWidth", UintegerValue(width), "MinX", DoubleValue(x), "MinY", DoubleValue(ymin), "DeltaX", DoubleValue(deltaX), "DeltaY", DoubleValue(deltaY));
	pedestrianMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	pedestrianMobility.Install(pedestrianNodes);

    //Assign IP addresses to pedestrian UEs
    NetDeviceContainer pedestrianLteDevs = lteHelper->InstallUeDevice(pedestrianNodes);
    internet.Install(pedestrianNodes);
    Ipv4InterfaceContainer pedestrianIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(pedestrianLteDevs));
    for(uint32_t i = 0; i < pedestrianNodes.GetN(); ++i) {
        Ptr<Node> pedestrianNode = pedestrianNodes.Get(i);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(pedestrianNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }
    lteHelper->Attach(pedestrianLteDevs, enbLteDevs.Get(0));

    for(uint32_t i = 0; i < pedestrianNodes.GetN(); ++i) {
    	Ptr<LteUeNetDevice> pedestrianLte = pedestrianLteDevs.Get(i)->GetObject<LteUeNetDevice>();
        Ptr<LteUePhy> pedestrianPhy = pedestrianLte->GetPhy();
        pedestrianPhy->SetAttribute("TxPower", DoubleValue(26+5));
    }

    //pedestrian applications
    ApplicationContainer pedestrianServerApps;
    dlPort = 1000;
    ulPort = 2000;
    for(uint32_t i = 0; i < pedestrianNodes.GetN(); ++i) {
    	ulPort++;
    	PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory", int);
    	PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
    	dlPacketSinkHelper.Install(pedestrianNodes.Get(i));
    	ulPacketSinkHelper.Install(remoteServer);

    	Ptr<UniformRandomVariable> onRandom = CreateObject<UniformRandomVariable>();
    	onRandom->SetAttribute("Min", DoubleValue(1.0));
    	onRandom->SetAttribute("Max", DoubleValue(7.0));

    	Ptr<UniformRandomVariable> offRandom = CreateObject<UniformRandomVariable>();
    	offRandom->SetAttribute("Min", DoubleValue(0));
    	offRandom->SetAttribute("Max", DoubleValue(4.0));
//    	offRandom->SetAttribute("Max", DoubleValue(0));
    	
    	Address ulAddress(InetSocketAddress(remoteServerAddr, ulPort));
    	OnOffHelper ulClient("ns3::UdpSocketFactory", ulAddress);
    	ulClient.SetAttribute("DataRate", DataRateValue(DataRate("150kbps")));
    	ulClient.SetAttribute("OnTime", PointerValue(onRandom));
    	ulClient.SetAttribute("OffTime", PointerValue(offRandom));

    	ApplicationContainer ulApp = ulClient.Install(pedestrianNodes.Get(i));

    	Address dlAddress(InetSocketAddress(pedestrianIpIfaces.GetAddress(i), dlPort));
    	OnOffHelper dlClient("ns3::UdpSocketFactory", dlAddress);
    	dlClient.SetAttribute("DataRate", DataRateValue(DataRate("1.5Mbps")));
    	dlClient.SetAttribute("OnTime", PointerValue(onRandom));
    	dlClient.SetAttribute("OffTime", PointerValue(offRandom));

    	ApplicationContainer dlApp = dlClient.Install(remoteServer);


    	int ulOffset = rand() % 2000;
    	ulApp.Get(0)->SetStartTime(MilliSeconds(ulOffset));
    	int dlOffset = rand() % 2000;
    	dlApp.Get(0)->SetStartTime(MilliSeconds(dlOffset));
    }

//    lteHelper->EnablePhyTraces();
//    lteHelper->EnablePdcpTraces();
//    lteHelper->EnableTraces();
//
//    pGWp2p.EnablePcapAll("lte");

//    AnimationInterface anim("v2x-mobility-trace.xml");
//    anim.SetConstantPosition(pgw, enbPosition.x + 2000, enbPosition.y, 0);
//    anim.SetConstantPosition(remoteV2XServer, enbPosition.x + 6000, enbPosition.y - 2000, 0);
//    anim.SetConstantPosition(remoteServer, enbPosition.x + 6000, enbPosition.y + 2000, 0);
////    anim.UpdateNodeDescription(remoteV2XServer, "remote V2X server");
////    anim.UpdateNodeDescription(remoteServer, "remote server");
////    anim.UpdateNodeDescription(router, "router");
////    anim.UpdateNodeDescription(pgw, "PGW");
////    anim.UpdateNodeDescription(enbNodes.Get(0), "eNB");
//    anim.UpdateNodeSize(pgw->GetId(), 100, 100);
//    anim.UpdateNodeSize(enbNodes.Get(0)->GetId(), 100, 100);
//    anim.UpdateNodeSize(remoteV2XServer->GetId(), 100, 100);
//    anim.UpdateNodeSize(remoteServer->GetId(), 100, 100);

    for(int i = 10; i < duration; i+=10) {
    	Simulator::Schedule(Seconds(i), &printCurrentTime);
    }

    //Trace Flows
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();
    flowMonitor->Start(Seconds(500));

    Simulator::Stop(Seconds(duration));
    Simulator::Run();

    flowMonitor->SerializeToXmlFile("lte-flow.xml", true, true);
    //Calculate Results
    //Vehicles
    double ulThroughput=0, dlThroughput=0, ulDelay = 0, dlDelay = 0, ulPDR = 0, dlPDR = 0;
    int ulFlows = 0, dlFlows = 0, ulRxPackets = 0, ulTxPackets = 0, dlRxPackets = 0, dlTxPackets = 0, ulLostPackets = 0, dlLostPackets = 0;
    //Pedestrians
    double ulPedDelay = 0, dlPedDelay = 0, ulPedPDR = 0, dlPedPDR = 0;
    int ulPedFlows = 0, dlPedFlows = 0, ulPedRxPackets = 0, ulPedTxPackets = 0, dlPedRxPackets = 0, dlPedTxPackets = 0, ulPedLostPackets = 0, dlPedLostPackets = 0;
    //emergency flows
    double emergencyDelay[numOfEmergencies], emergencyUlDelay[numOfEmergencies], emergencyDlDelay[numOfEmergencies], emergencyDlFlows[numOfEmergencies];
    for(int i = 0; i < numOfEmergencies; ++i) {
    	emergencyUlDelay[i] = 0;
    	emergencyDlDelay[i] = 0;
    	emergencyDlFlows[i] = 0;
    }

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
    	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
    	if(t.destinationPort == emergencyUlPort) {
    		//emergency uplink message
    		int emergencyIndex = findEmergencyNumber(iter->second.timeFirstTxPacket, emergencyTimes, numOfEmergencies);
    		if(iter->second.rxPackets !=0 ) {
        		emergencyUlDelay[emergencyIndex] = iter->second.delaySum.ToDouble(Time::MS) / iter->second.rxPackets;
    		}
    	} else if(t.destinationPort == emergencyDlPort) {
    		//emergency downlink message
    		int emergencyIndex = findEmergencyNumber(iter->second.timeFirstTxPacket, emergencyTimes, numOfEmergencies);
    		if(iter->second.rxPackets != 0) {
        		emergencyDlDelay[emergencyIndex] += iter->second.delaySum.ToDouble(Time::MS) / iter->second.rxPackets;
        		emergencyDlFlows[emergencyIndex]++;
    		}
    	} else if(iter->second.rxPackets > 1) {//periodic message with enough packets to calculate throughput
    		if(isVehicle(t.sourceAddress, vehicleIpIfaces)) { //Src is a vehicle
    		    ulThroughput += iter->second.rxBytes * 8.0 /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000;
    		    ulDelay += iter->second.delaySum.ToDouble(Time::MS);
    		    ulTxPackets += iter->second.txPackets;
    		    ulRxPackets += iter->second.rxPackets;
    		    ulLostPackets += iter->second.lostPackets;
    		    ulFlows++;
    		} else if(t.sourceAddress == remoteV2XAddr){ //Src is the v2x server
    		    dlThroughput += iter->second.rxBytes * 8.0 /(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1000;
    		    dlDelay += iter->second.delaySum.ToDouble(Time::MS);
    		    dlTxPackets +=iter->second.txPackets;
    		    dlRxPackets += iter->second.rxPackets;
    		    dlLostPackets += iter->second.lostPackets;
    		    dlFlows++;
    		} else if(t.destinationAddress == remoteServerAddr) { //UL pedestrian flows
    			ulPedDelay += iter->second.delaySum.ToDouble(Time::MS);
    			ulPedTxPackets += iter->second.txPackets;
    			ulPedRxPackets += iter->second.rxPackets;
    			ulPedLostPackets += iter->second.lostPackets;
    			ulPedFlows++;
    		} else if(t.sourceAddress == remoteServerAddr) { //DL pedestrian flows
    			dlPedDelay += iter->second.delaySum.ToDouble(Time::MS);
    			dlPedTxPackets += iter->second.txPackets;
    			dlPedRxPackets += iter->second.rxPackets;
    			dlPedLostPackets += iter->second.lostPackets;
    			dlPedFlows++;
    		
    	}

    }
    if(ulFlows != 0) ulThroughput /= ulFlows;
    if(dlFlows != 0) dlThroughput /= dlFlows;
    if(ulRxPackets != 0) ulDelay /= ulRxPackets;
    if(dlRxPackets != 0) dlDelay /= dlRxPackets;
    if(ulTxPackets != 0) ulPDR =  100.0 * (1 - ulLostPackets * 1.0 / ulTxPackets);
    if(dlTxPackets != 0) dlPDR = 100.0 * (1 - dlLostPackets * 1.0/ dlTxPackets);
    std::cout << "***Vehicle results***\n";
    std::cout << "Average DL throughput: " << dlThroughput << " kbps\n";
    std::cout << "Average UL throughput: " << ulThroughput << " kbps\n";
    std::cout << "Average DL delay: " << dlDelay << " ms\n";
    std::cout << "Average UL delay: " << ulDelay << " ms\n";
    std::cout << "Average DL PDR: " << dlPDR << "%\n";
    std::cout << "Average UL PDR: " << ulPDR << "%\n";

    std::cout << "\n***Emergency results***\n";
    for(int i = 0; i < numOfEmergencies; ++i) {
    	std::cout << "Event at " << emergencyTimes[i] << "sec\n";
        if(emergencyDlFlows[i] != 0) {
        	emergencyDelay[i] = emergencyUlDelay[i] + emergencyDlDelay[i] / emergencyDlFlows[i];
        }
        std::cout << "Average Delay: " << emergencyDelay[i] << " ms\n";
        std::cout << "Number of vehicles that received the message: " << emergencyDlFlows[i] << "\n\n";
    }

    if(ulPedRxPackets != 0) ulPedDelay /= ulPedRxPackets;
    if(dlPedRxPackets != 0) dlPedDelay /= dlPedRxPackets;
    if(ulPedTxPackets != 0) ulPedPDR =  100.0 * (1 - ulPedLostPackets * 1.0 / ulPedTxPackets);
    if(dlPedTxPackets != 0) dlPedPDR = 100.0 * (1 - dlPedLostPackets * 1.0/ dlPedTxPackets);
    std::cout << "\n***Pedestrian results***\n";
    std::cout << "Average DL delay: " << dlPedDelay << " ms\n";
    std::cout << "Average UL delay: " << ulPedDelay << " ms\n";
    std::cout << "Average DL PDR: " << dlPedPDR << "%\n";
    std::cout << "Average UL PDR: " << ulPedPDR << "%\n";
    Simulator::Destroy();

    return 0;
}
