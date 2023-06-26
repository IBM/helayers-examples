from pandas._libs import missing
from sklearn.compose import make_column_transformer
from sklearn.compose import make_column_selector
from sklearn.preprocessing import (
    OrdinalEncoder,
    KBinsDiscretizer,
    LabelEncoder,
    OneHotEncoder,
)
from sklearn.compose import ColumnTransformer
import pandas as pd
import numpy as np


class Preprocessor:
    """
    Preprocessor for binary classification data for CRF training by converting it to one-hot representation
    and generating predictor_description that defines partition of the original features
    """

    def __init__(self, num_bins=10, batch_size = -1):
        """Initiate Preprocessor with predefined number of encoding bins for numerical data

        Args:
            num_bins (int, optional): number of encoding bins for numerical data. Defaults to 10.
            batch_size (in, optional) the size of the batches in which the
            training and testing data will be preprocessed. Defaults to -1, which
            means all data will be preprocessed once.
        """
        self.num_bins = num_bins
        self.batch_size = batch_size

        self._reset_state()

    def _reset_state(self):
        self.cat_encoders = []
        self.label_encoder = LabelEncoder()

        self.discretizers = []
        self.columns_to_merge = []
        self.columns_list = []
        self.output_columns = []
        self.trained = False
        self.batch_ind = 0
        
    def fit_transform( self, X: pd.DataFrame, y: pd.DataFrame):
        X_prep, y_prep, cat_pred, ord_pred = self._preprocess(X,y, is_train_set=True)
        return X_prep,y_prep, cat_pred, ord_pred

    def transform_next_batch( self, X: pd.DataFrame, y: pd.DataFrame):
        """Iteratively transform a batch of non-binary data to binary data with
        one-hot encoding. Returns X_res, y_res, last_batch. Where X_res, y_res
        stand for the transformed batch, and last_batch is a flag specifying
        whether we finished transforming the whole data.

        Args:
            X (pd.DataFrame): A set of samples.
            y (pd.DataFrame): A set of labels.
        """
        if self.batch_size == -1:
            self.batch_size = X.shape[0]
        if self.batch_ind == 0:
            self.label_encoder.fit(y)
        batch_begin = self.batch_size * self.batch_ind
        batch_end = min(batch_begin + self.batch_size, X.shape[0])
        last_batch = (batch_end == X.shape[0])
        X_batch = X.iloc[batch_begin:batch_end,:]
        y_batch = y.iloc[batch_begin:batch_end]
        self.batch_ind = self.batch_ind + 1
        X_res, y_res = self.transform(X_batch, y_batch)
        if last_batch:
            self.batch_ind = 0
        return X_res, y_res, last_batch

    def transform( self, X: pd.DataFrame, y: pd.DataFrame):
        X_prep, y_prep, _, _ = self._preprocess(X,y,is_train_set=False)
        return X_prep, y_prep

    def preprocess_predictor_descriptions(self, X: pd.DataFrame):
        """Get the predictor descriptions to be used to intialized a Crf object.

        Args:
            X (pd.DataFrame): A set of samples.
        """
        self._set_categorical_and_ordinal_features(X, True)
        self._categories_preprocessing(X, self.columns_list)
        self._numerical_feature_ranges_preprocessing(X, self.columns_list)
        cat_pred, ord_pred  = self._get_predictor_descriptions(self.columns_list)
        self.trained = True
        return cat_pred, ord_pred
    def _preprocess(self, X: pd.DataFrame,  y: pd.DataFrame, is_train_set: bool=False):
        """apply preprocessing on input data, the first call to this method should be done with is_train_set=True

        Args:
            X (pd.DataFrame): input features
            y (pd.DataFrame): input labels
            is_train_set (bool, optional): defines if the input data is the training set. Defaults to False.

        Returns:
            X_prep(pd.DataFrame), y_prep(pd.DataFrame), categorial_predictors(list), oridnal_predictors(list):
                X_prep and y_prep are one-hot representation of input data
                categorial_predictors is list of one-hot indexes for categorial features
                oridnal_predictors is list of one-hot indexes for oridnal features
        """
        try:
            if is_train_set:
                self._reset_state()

            elif not self.trained:
                raise Exception(
                    "The first call to preprocess should be done with is_train_set=True"
                )

            X_prep, rows_with_new_categories = self._preprocess_X(X, is_train_set)
            self.rows_with_new_categories = rows_with_new_categories
            
            y_prep = self._preprocess_y(y, is_train_set)

            cat_pred, ord_pred  = self._get_predictor_descriptions(X_prep.columns)

        except Exception as e:
            if is_train_set:
                self.trained = False
            raise e

        if not self.trained:
            self.trained = True

        return X_prep.to_numpy(), y_prep, cat_pred, ord_pred

    def _preprocess_y(self, y, to_train):
        if to_train:
            self.label_encoder.fit(y)

        y = self.label_encoder.transform(y)
        y = y.reshape(-1,1)
        return y

    def _preprocess_X(self, X, to_train):
        self._set_categorical_and_ordinal_features(X, to_train)

        dfs = []
        rows_with_new_categories = self._categorical_features_preprocessing(X, to_train, dfs)
        self._numerical_features_preprocessing(X, to_train, dfs)
        X_full = self._unite_preprocessed_features(to_train, dfs)
        return X_full, rows_with_new_categories

    def _unite_preprocessed_features(self, to_train, dfs):
        X_full = pd.concat(dfs, axis=1)
        if to_train or len(self.output_columns) == 0:
            self.output_columns = X_full.columns
        else:

            new_columns = X_full.columns.difference(self.output_columns)
            if len(new_columns) > 0:
                print(
                    f"Detected new {len(new_columns)} columns that were not in the training - dropping\n"
                    f"{list(new_columns)[:10]} ..."
                )
                X_full = X_full.drop(columns=new_columns)

            missing_columns = self.output_columns.difference(X_full.columns)
            if len(missing_columns) > 0:
                print(
                    f"Detected {len(missing_columns)} missing columns missing in the training - adding\n"
                    f"{list(new_columns)[:10]} ..."
                )
                X_full[new_columns] = 0

            X_full = X_full[self.output_columns]
        return X_full

    def _numerical_feature_ranges_preprocessing(self, X, columns_list):
        Xnum = X[self.num_ix]
        #X_num_truncated = Xnum.iloc[:self.batch_size,:]
        for col_id, col in enumerate(Xnum.columns):
            Xnum_oh = self._process_numerical_feature(True, Xnum, col_id, col)
            if Xnum_oh.shape[1] <= 1:
                print(
                    f"Total number of columns for {col} is {Xnum_oh.shape[1]} - skipping this variable"
                )
            else:
                for col in Xnum_oh.columns:
                    columns_list.append(col)

    def _numerical_features_preprocessing(self, X, to_train, dfs):
        Xnum = X[self.num_ix]
        for col_id, col in enumerate(Xnum.columns):
            Xnum_oh = self._process_numerical_feature(to_train, Xnum, col_id, col)
            if Xnum_oh.shape[1] <= 1:
                print(
                    f"Total number of columns for {col} is {Xnum_oh.shape[1]} - skipping this variable"
                )
            else:
                dfs.append(Xnum_oh)

    def _process_numerical_feature(self, to_train, Xnum, col_id, col):

        Xnum_oh = self._encode_numerical_feature(to_train, Xnum, col_id, col)
        Xnum_oh = self._merge_not_used_ranges(Xnum_oh, col_id, to_train)
        Xnum_oh = self._apply_numerical_naming_convention(col, Xnum_oh)
        return Xnum_oh

    def _apply_numerical_naming_convention(self, col, Xnum_oh):
        Xnum_oh.columns = [f"{col}__ord_{n}" for n, _ in enumerate(Xnum_oh.columns)]
        Xnum_oh = Xnum_oh.astype(int)
        return Xnum_oh

    def _encode_numerical_feature(self, to_train, Xnum, col_id, col):
        if to_train:
            discretizer = KBinsDiscretizer(
                n_bins=self.num_bins, encode="onehot-dense", strategy="quantile"
            )
            discretizer.fit(
                Xnum[
                    [
                        col,
                    ]
                ]
            )
            # swithing to uniform strategy if KBinsDiscretizer quantile strategy produces one or zero bins
            if discretizer.n_bins_[0] <= 1:
                discretizer = KBinsDiscretizer(
                    n_bins=self.num_bins,
                    encode="onehot-dense",
                    strategy="uniform",
                )
                discretizer.fit(
                    Xnum[
                        [
                            col,
                        ]
                    ]
                )

            self.discretizers.append(discretizer)
        else:
            discretizer = self.discretizers[col_id]

        Xnum_oh = pd.DataFrame(discretizer.transform(Xnum[[col]]))

        return Xnum_oh

    def _merge_not_used_ranges(self, Xnum_oh, col_id, to_train):
        if to_train:
            empty_ranges = Xnum_oh.sum(axis=0) == 0
            empty_columns = Xnum_oh.columns[empty_ranges]
            self.columns_to_merge.append(empty_columns)
        else:
            empty_columns = self.columns_to_merge[col_id]

        num_empty_columns = len(empty_columns)

        if num_empty_columns > 0:
            last_good = 0 
            for i_col, n_column in enumerate (Xnum_oh.columns):
                if n_column in empty_columns:
                    Xnum_oh.iloc[:,last_good] += Xnum_oh.iloc[:,i_col]
                else:
                    last_good = i_col

            Xnum_oh = Xnum_oh.drop(columns=empty_columns)
        return Xnum_oh
    
    def _categories_preprocessing(self, X, columns_list):
        Xcat = X[self.cat_ix]
        Xcat = Xcat.fillna("NA")
        for col in Xcat.columns:
            oh_encode = OneHotEncoder(
                handle_unknown="ignore", sparse=False, dtype=np.int
            )
            oh_encode.fit(Xcat[[col]])
            self.cat_encoders.append(oh_encode)
            categories = oh_encode.categories_
            for col_num in range(len(categories[0])):
                columns_list.append(f"{col}__cat_{col_num}")

    def _categorical_features_preprocessing(self, X, to_train, dfs):
        Xcat = X[self.cat_ix]
        Xcat = Xcat.fillna("NA")
        if to_train:
            for col in Xcat.columns:
                oh_encode = OneHotEncoder(
                    handle_unknown="ignore", sparse=False, dtype=np.int
                )
                oh_encode.fit(Xcat[[col]])
                self.cat_encoders.append(oh_encode)
        rows_with_new_categories = set()
        for ind_col, col in enumerate(Xcat.columns):
            Xoh = self.cat_encoders[ind_col].transform(Xcat[[col]])
            new_cat_ind = np.where(Xoh.sum(axis=1)==0)[0]
            if len(new_cat_ind)>0:
                rows_with_new_categories.update(new_cat_ind)
            Xoh = pd.DataFrame(
                Xoh,
                columns=[f"{col}__cat_{col_num}" for col_num in range(Xoh.shape[1])],
            )
            if Xoh.shape[1] <= 1:
                print(
                    f"Total number of categories for {col} is  <=1 ({Xoh.shape[1]}) - skipping this variable"
                )
            else:
                dfs.append(Xoh)
        return list(rows_with_new_categories)

    def _set_categorical_and_ordinal_features(self, X, to_train):
        if to_train:
            self.input_columns = X.columns
            self.cat_ix = X.select_dtypes(include=["object", "bool"]).columns
            self.num_ix = X.select_dtypes(include=["int64", "float64"]).columns
        else:
            if np.any(X.columns != self.input_columns):
                raise Exception(
                    f"Data columns should be exactly those used for the training:"
                    f" \n training columns: {self.input_columns}"
                    f" \n current columns: {X.columns}"
                )

    def _get_predictor_descriptions(self, columns):
        """

        Args:
            columns (list): list of the columns using special naming convention

        Returns:
            list: list of predictor descriptions used by CRF
        """

        features_dict = {}

        for cl_num, cl in enumerate(columns):

            name, f_type = self._parse_column_name(cl)

            if name not in features_dict:
                features_dict[name] = {
                    "feature_names": [],
                    "feature_indexes": set(),
                    "feature_type": f_type,
                }

            features_dict[name]["feature_names"].append(cl)
            features_dict[name]["feature_indexes"].add(cl_num)
            assert columns[cl_num] == cl, f"{cl_num} != {cl}"

        self.features_dict = features_dict
        cat_pred, ord_pred  = self._prep_predictors()
        return  cat_pred, ord_pred 

    def _parse_column_name(self, cl):
        name_suffix = cl.split("__")
        assert len(name_suffix) == 2, f"Wrong format1 {name_suffix}"
        name = name_suffix[0]
        ptype_num = name_suffix[1].split("_")
        assert len(ptype_num) == 2, f"Wrong format2 {name_suffix}"
        ftype = ptype_num[0]

        return name, ftype

    def _prep_predictors(self):

        predictors = dict(ord=[],cat=[])
        
        for varaible in self.features_dict.values():
            predictors[varaible["feature_type"]].append(varaible["feature_indexes"]) 

        return predictors["cat"], predictors["ord"]
