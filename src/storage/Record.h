#pragma once
 
// 44 bytes per record (40 + 4 bytes padding)
struct Record {
    char game_date[11]; // 11 bytes
    int team_id_home; // 4 bytes
    int pts_home; // 4 bytes
    float fg_pct_home; // 4 bytes
    float ft_pct_home; // 4 bytes
    float fg3_pct_home; // 4 bytes
    int ast_home; // 4 bytes
    int reb_home; // 4 bytes
    bool home_team_wins; // 1 byte
};
