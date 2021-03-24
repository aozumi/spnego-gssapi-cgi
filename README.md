# GSSAPI認証 CGI

web ブラウザによるアクセスを Kerberos認証 (GSSAPI/SPNEGO) する
コードのCGIによる実装実験。

 * Kerberos実装: Heimdal

## ビルド

```
% bmake
```
完了すると `spnegocgi` コマンドが作られる。

## テスト

Docker によるテストができる。

必要な条件:
 * 機能しているKerberos環境
 * Dockerマシンにホスト名(FQDN)がついていること
 * Dockerコンテナ用の `krb5.conf` ファイル (なければビルド時にローカルの `/etc/krb5.conf` を流用する)
 * テスト用のサービスプリンシパル `HTTP/ホスト名`
 * つぎのいずれか
   * そのクレデンシャルを格納したkeytabファイル `http.keytab`
   * または、それを作成するための管理者プリンシパル

```
% bmake run-docker
```
とすると、Docker イメージをビルドしてコンテナを実行する。

ビルド時のパラメータとして以下の変数がある:
 * `ADMIN_PRINCIPAL` --- keytabを作成するための管理者プリンシパル。デフォルトは `ユーザ名/admin`
 * `HOSTNAME` --- コンテナを実行するDockerマシンのホスト名 (FQDN)。
 * `PORT` --- コンテナがHTTPで待ち受けるポート番号。デフォルトは8089。

ブラウザで `http://(Dockerホスト名):8089/` を開くとCGIにリダイレクトされ、SPNEGO authentication succeeded! と表示されれば成功。

## GSSAPIで認証するためのブラウザの設定

Firefox では `about:config` を開いて次の項目を設定します。

* `network.negotiate-auth.trusted-urls` --- `.example.com` (GSSAPI認証したいドメイン。複数の場合はコンマで区切って並べる)
* `network.negotiate-auth.delegation-urls` --- `.example.com` (GSSAPI認証したいドメイン。複数の場合はコンマで区切って並べる)
* `network.auth.use-sspi` --- `false`

Google Chrome (Linux版) の場合は `/etc/opt/chrome/policies/managed/mydomain.json` などの名前で次の内容のファイルを作成する。

```
{
  "AuthServerAllowlist": ".example.com",
  "AuthServerDelegateAllowlist": ".example.com"
}
```
