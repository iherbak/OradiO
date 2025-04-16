import { OSettings, Playlist} from "models";

export class ApiService {

    async getPlaylist(): Promise<Playlist> {
        const response = await fetch('api/playlist');
        let data = await response.json();
        return data;
    };

    async getSettings(): Promise<OSettings> {
        const response = await fetch('api/settings');
        let data = await response.json();
        return data;
    };

    async saveSettings(settings : OSettings): Promise<boolean> {
        const response = await fetch('api/settings',{ method:"POST",body: JSON.stringify(settings)});
        return response.ok;
    };

    async restart(): Promise<boolean> {
        const response = await fetch('api/restart',{ method:"POST",body: ""});
        return response.ok;
    };

    async savePlaylist(playlist : Playlist): Promise<boolean> {
        const response = await fetch('api/playlist',{ method:"POST",body: JSON.stringify(playlist)});
        return response.ok;
    };

    async changeToStation(stationdId: number): Promise<number> {
        const response = await fetch('api/setstation',
            {
                method: "POST",
                body: stationdId.toString()
            }
        )
        return await response.status;
    }


}