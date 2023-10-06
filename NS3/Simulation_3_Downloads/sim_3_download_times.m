clc;
clear all;
close all;

download_time_tcp = [0.419 0.566 0.674 0.768 0.926];
download_time_quic = [0.323 0.412 0.539 0.66 0.884];

%Plot Download Times vs File Size

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(download_time_tcp,'--o','Color','b','MarkerSize',10,'MarkerFaceColor','b','LineWidth',2.2);
plot(download_time_quic,'--o','Color','r','MarkerSize',10,'MarkerFaceColor','r','LineWidth',2.2);

xticklabels({'64kB','256kB', '512kB', '1MB', '2MB'});
 
xlabel('File Size','FontSize',20);
ylabel('Download Time (seconds)','FontSize',20);
title('Download Time Performance of TCP vs QUIC (UE_{radius} = 250m)','FontSize',22);

leg = legend('TCP','QUIC');
leg.FontSize = 18;
leg.Location = 'NorthWest';

hold off;
grid on;

