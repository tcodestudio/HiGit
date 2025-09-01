import { Context } from '@kit.AbilityKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { preferences } from '@kit.ArkData';
import { RepoItem } from '../data/RepoItem';
import { md5String } from '../utils/Utils';
import { hilog } from '@kit.PerformanceAnalysisKit';

export function initApp(context: Context): void {

  // 同步证书
  const cert_file: string = context.filesDir + "/" + "cert.pem";
  if (!fs.accessSync(cert_file)) {
    let data: Uint8Array = context.resourceManager.getRawFileContentSync("cacert.pem");
    let dest_file = fs.openSync(cert_file, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
    fs.writeSync(dest_file.fd, data.buffer);
    fs.closeSync(dest_file);
  }

  // 初次默认一个 demo 仓库
  let options: preferences.Options = { name: 'gitStores' } as preferences.Options;
  let init_repo = preferences.getPreferencesSync(context, options).getSync("init_repo", false) as boolean
  if (!init_repo) {
    hilog.info(0x0000, "appTag", "add init repo");
    addRepo(context, "https://gitcode.com/neosu/HiGit.git")
    preferences.getPreferencesSync(context, options).putSync("init_repo", true);
    preferences.getPreferencesSync(context, options).flush(() => {
      context.eventHub.emit("reload_app_list", "refresh")
    });
  }
}

let reposData = "";

function getRepos(context: Context): string {
  let options: preferences.Options = { name: 'gitStores' } as preferences.Options;
  let data = preferences.getPreferencesSync(context, options).getSync("repos", "[]") as string;
  reposData = data;
  return data;
}

function setRepos(context: Context, data: string): void {
  if (reposData === data) {
    hilog.info(0x0000, "appTag", "save repos data but not change");
    return;
  }
  hilog.info(0x0000, "appTag", "save repos data");
  let options: preferences.Options = { name: 'gitStores' } as preferences.Options;
  let dataPreference = preferences.getPreferencesSync(context, options);
  dataPreference.putSync("repos", data);
  dataPreference.flush(() => {
    context.eventHub.emit("reload_app_list", "refresh")
  });
}

function getRepoList(context: Context): Array<RepoItem> {
  let data: string = getRepos(context);
  try {
    let repoList: Array<RepoItem> = JSON.parse(data) as Array<RepoItem>;
    return repoList;
  } catch (error) {
    console.error('解析仓库数据失败:', error);
    return [];
  }
}

function saveRepoList(context: Context, repoList: Array<RepoItem>): void {
  let data: string = JSON.stringify(repoList);
  setRepos(context, data);
}

export function addRepo(context: Context, url: string): boolean {
  try {
    let repoList: Array<RepoItem> = getRepoList(context);

    // 检查是否已存在相同 URL 的仓库
    let existingRepo: RepoItem | undefined = repoList.find(repo => repo.url === url);
    if (existingRepo) {
      hilog.info(0x0000, "appTag", "仓库已存在: %{public}s", url);
      return false;
    }

    let repo: RepoItem = new RepoItem(url);
    repoList.push(repo);
    saveRepoList(context, repoList);
    return true;
  } catch (error) {
    hilog.info(0x0000, "appTag", "添加仓库失败: %{public}s", error);
    console.error('添加仓库失败:', error);
    return false;
  }
}

export function updateRepo(context: Context, url: string, updates: RepoItem): boolean {
  try {
    let repoList: Array<RepoItem> = getRepoList(context);
    let repoIndex: number = repoList.findIndex(repo => repo.url === url);

    if (repoIndex === -1) {
      console.warn('仓库不存在:', url);
      return false;
    }

    // 更新仓库属性
    let repo: RepoItem = repoList[repoIndex];
    repo.name = updates.name;
    repo.provider = updates.provider;
    repo.activity = updates.activity;
    repo.branch = updates.branch;
    repo.time = updates.time;
    repo.branches = updates.branches;
    repo.tags = updates.tags;
    repo.commits = updates.commits;
    saveRepoList(context, repoList);
    return true;
  } catch (error) {
    console.error('更新仓库失败:', error);
    return false;
  }
}

export function removeRepo(context: Context, repoURL: string): boolean {
  try {
    let repoList: Array<RepoItem> = getRepoList(context);
    let repoIndex: number = repoList.findIndex(repo => repo.url.trim() === repoURL.trim());

    if (repoIndex === -1) {
      hilog.warn(0x0000, "appTag", "仓库不存在:%{public}s", repoURL);
      return false;
    }

    repoList.splice(repoIndex, 1);
    saveRepoList(context, repoList);
    return true;
  } catch (error) {
    hilog.warn(0x0000, "appTag", "删除仓库失败:%{public}s", repoURL);
    return false;
  }
}

export function getRepoById(context: Context, repoId: string): RepoItem | null {
  try {
    let repoList = getRepoList(context);
    return repoList.find(repo => repo.id === repoId) || null;
  } catch (error) {
    console.error('获取仓库失败:', error);
    return null;
  }
}

export function getRepoByUrl(context: Context, url: string): RepoItem | null {
  return getRepoById(context, md5String(url))
}

export function getAllRepos(context: Context): Array<RepoItem> {
  return getRepoList(context);
}

export function clearAllRepos(context: Context): boolean {
  try {
    setRepos(context, "[]");
    return true;
  } catch (error) {
    console.error('清空仓库失败:', error);
    return false;
  }
}