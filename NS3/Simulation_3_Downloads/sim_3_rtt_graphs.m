clc;
clear all;
close all;

table_1 = readtable('senderTcp-cwnd-change.csv');
array_1 = table2array(table_1);

table_2 = readtable('senderQUIC-cwnd-change7.csv');
array_2 = table2array(table_2);

time_1 = array_1(:,1);
func_1 = array_1(:,2);

time_2 = array_2(:,1);
func_2 = array_2(:,2);

%Plot Congestion Window vs Time

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(time_1,func_1(:,1),'-','Color','b','LineWidth',2.2);
plot(time_2,func_2(:,1),'-','Color','r','LineWidth',2.2);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Congestion Window (kB)','FontSize',20);
title('RTT Estimation of TCP vs QUIC for download of X file','FontSize',22);

hold off;
grid on;

