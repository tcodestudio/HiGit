export const initSystem: (path: string) => { success: number, message: string, data: string };

export const initRepo: (path: string, url: string, repo: string,
  provider: string) => { success: number, message: string, data: string };

export const getBranches: (url: string) => { success: number, message: string, data: string };

export const getTags: (url: string) => { success: number, message: string, data: string };

export const fetch: (url: string, branch: string, callback: (process: number, total: number,
  message: string) => void) => { success: number, message: string, data: string };

export const history: (url: string, branch: string, count: number,
  offset: number) => { success: number, message: string, data: string };

export const getSSHKey: () => { success: number, message: string, data: string };

export const generateSSHKey: () => { success: number, message: string, data: string };

export const deleteRepo: (path: string, url: string, repo: string,
  provider: string) => { success: number, message: string, data: string }

export const getFileTree: (url: string, branch: string) => { success: number, message: string, data: string };

export const readFile: (url: string, branch: string, path: string) => { success: number, message: string, data: string };