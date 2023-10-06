clc;
clear all;
close all;

table_1 = readtable('2_QUIC_2_TCP/Tcp_Throughput_Calculations.csv');
array_1 = table2array(table_1);

time_1 = array_1(:,1);
tcp_throughputs = array_1(:,(2:end));

table_2 = readtable('2_QUIC_2_TCP/Quic_Throughput_Calculations.csv');
array_2 = table2array(table_2);

time_2 = array_2(:,1);
quic_throughputs = array_2(:,(2:end));


%Plot Throughput vs Time for all flows

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(time_1,tcp_throughputs(:,1),'-','Color','k','LineWidth',2.2);
plot(time_1,tcp_throughputs(:,2),'-','Color','b','LineWidth',2.2);
%plot(time_1,tcp_throughputs(:,3),'-','Color','b','LineWidth',2.2);
%plot(time_1,tcp_throughputs(:,4),'-','Color','g','LineWidth',2.2);
%plot(time_1,tcp_throughputs(:,5),'-','Color','m','LineWidth',2.2);


plot(time_2,quic_throughputs(:,1),'-','Color','r','LineWidth',2.2);
plot(time_2,quic_throughputs(:,2),'-','Color','g','LineWidth',2.2);

xlim([0 40]);
ylim([0 8]);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Throughput (Mbps)','FontSize',20);
title('Throughput fairness for competing flows over the simulated LTE network (UE_{radius} = 250m)','FontSize',22);


%leg = legend('QUIC-1','QUIC-2');
%leg = legend('TCP-1','TCP-2','QUIC');
%leg = legend('TCP-1','TCP-2','TCP-3','TCP-4','TCP-5','QUIC');
leg = legend('TCP-1','TCP-2','QUIC-1','QUIC-2');

leg.FontSize = 18;

hold off;
grid on;

