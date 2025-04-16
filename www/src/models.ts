
export class Playlist {
    defaultStation: number = -1;
    stations: Station[] = [];
}

export class Station {
    id: number;
    url: string;
    desc: string

    constructor(i: number, u: string, d: string) {
        this.id = i;
        this.url = u;
        this.desc = d;
    }
}

export class changeStationReq {
    constructor(_id: number) {
        this.id = _id;
    }
    id: number;
}

export class AppState {
    visibleContent: string;
    playlist: Playlist;
    stationValue: number;
    stationFilter: string;
    filteredStations: Station[];
    showNewCard: boolean;
    newStation: Station;
    editStationId: number;
    alerts: Alert[];
    settings: OSettings;
}

export class PlayerProps {
    playlist: Playlist;
    stationValue: number;
    setPlaylist: Function;
    switchStation: Function;
}

export class StationsProps {
    clearAlerts: Function;
    addAlert: Function;
    setPlaylist: Function;
    setFilter: Function;
    addStation: Function;
    updateNewDesc: Function;
    showAddStation: Function;
    cancelEdit: Function;
    updateNewUrl: Function;
    editStation: Function;
    deleteStation: Function;
    setStationAsDefault: Function;
    state: AppState;
}

export class SettingsProps {
    settings: OSettings;
    setSettings: Function;
    getSettings: Function;
    state: AppState;
    clearAlerts: Function;
    addAlert: Function;
}

export class Alert {
    content: string;
    timeout: number;
    dismiss: Function;
    type: string;
}


export class DisplaySettings {
    sdapin: number;
    sclpin: number;
    rows: number;
    cols: number;
}

export class Vs1053Settings {
    dreqpin: number;
    xcspin: number;
    xdcspin: number;
    resetpin: number;
    mosipin: number;
    misopin: number;
    sclkpin: number;
}

export class WifiSettings {
    stassid: string;
    stapassword: string;
    apssid: string;
    appassword: string;
    startasap: boolean;
}

export class OSettings {
    display: DisplaySettings;
    vs1053: Vs1053Settings;
    wifi: WifiSettings;
}

export enum SettingsField {
    stassid,
    stapassword,
    apssid,
    appassword,
    startasap,
    rows,
    cols,
    sdapin,
    sclpin,
    dreqpin,
    xcspin,
    xdcspin,
    resetpin,
    mosipin,
    misopin,
    sclkpin
}

