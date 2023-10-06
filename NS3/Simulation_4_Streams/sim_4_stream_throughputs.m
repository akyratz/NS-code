clc;
clear all;
close all;


download_speed_quic_1_stream = [4.336 4.336 4.336 4.336 4.336];
download_speed_quic_many_stream = [4.336 4.92 5.15 5.6 5.937];

%Plot Throughputs vs Number of Streams for different UE distances

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(download_speed_quic_1_stream,'--o','Color','b','MarkerSize',10,'MarkerFaceColor','b','LineWidth',2.5);
plot(download_speed_quic_many_stream,'--o','Color','r','MarkerSize',10,'MarkerFaceColor','r','LineWidth',2.5);

xticks([1 2 3 4 5]);
xticklabels({'1', '2', '4', '8', '16'});

ylim([4.2 6.0]);

xlabel('Number of QUIC Streams','FontSize',20);
ylabel('Throughput (Mbps)','FontSize',20);
title('Effect of QUIC streams on throughput (UE_{radius} = 250m)','FontSize',22);

leg = legend('QUIC - 1 Stream','QUIC - Multiple Streams');
leg.FontSize = 18;
leg.Location = 'NorthWest';

hold off;
grid on;

