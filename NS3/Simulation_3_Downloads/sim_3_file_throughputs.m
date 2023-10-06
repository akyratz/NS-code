clc;
clear all;
close all;

download_speed_tcp = [1.25128 3.70522 6.223 10.6667 17.6933];
download_speed_quic = [1.62318 5.09017 7.78164 12.4121 18.5339];

veltiwsh = ((download_speed_quic - download_speed_tcp)./download_speed_tcp)*100

%Plot Download Goodput vs File Size

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(download_speed_tcp,'--o','Color','b','MarkerSize',10,'MarkerFaceColor','b','LineWidth',2.2);
plot(download_speed_quic,'--o','Color','r','MarkerSize',10,'MarkerFaceColor','r','LineWidth',2.2);


xticklabels({'64kB','256kB', '512kB', '1MB', '2MB', '5MB', '10MB'});
 
xlabel('File Size','FontSize',20);
ylabel('Goodput (Mbps)','FontSize',20);
title('Goodput Performance of TCP vs QUIC (UE_{radius} = 250m)','FontSize',22);

leg = legend('TCP','QUIC');
leg.FontSize = 18;
leg.Location = 'NorthWest';

hold off;
grid on;

